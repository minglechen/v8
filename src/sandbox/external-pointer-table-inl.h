// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_SANDBOX_EXTERNAL_POINTER_TABLE_INL_H_
#define V8_SANDBOX_EXTERNAL_POINTER_TABLE_INL_H_

#include "src/base/atomicops.h"
#include "src/sandbox/external-pointer-table.h"
#include "src/sandbox/external-pointer.h"
#include "src/utils/allocation.h"

#ifdef V8_ENABLE_SANDBOX

namespace v8 {
namespace internal {

void ExternalPointerTable::Init(Isolate* isolate) {
  DCHECK(!is_initialized());

  VirtualAddressSpace* root_space = GetPlatformVirtualAddressSpace();
  DCHECK(IsAligned(kExternalPointerTableReservationSize,
                   root_space->allocation_granularity()));
  buffer_ = root_space->AllocatePages(
      VirtualAddressSpace::kNoHint, kExternalPointerTableReservationSize,
      // Need 
#if defined(__CHERI_PURE_CAPABILITY__)
      // VM_PROT_ADD_CAP is never called on prot and max_prot in mprotect itself.
      // https://github.com/CTSRD-CHERI/cheribsd/issues/1818
      root_space->allocation_granularity(), PagePermissions::kReadWrite);
#else // defined(__CHERI_PURE_CAPABILITY__)
      root_space->allocation_granularity(), PagePermissions::kNoAccess);
#endif // defined(__CHERI_PURE_CAPABILITY__)
  if (!buffer_) {
    V8::FatalProcessOutOfMemory(
        isolate,
        "Failed to reserve memory for ExternalPointerTable backing buffer");
  }

  mutex_ = new base::Mutex;
  if (!mutex_) {
    V8::FatalProcessOutOfMemory(
        isolate, "Failed to allocate mutex for ExternalPointerTable");
  }

  // Allocate the initial block. Mutex must be held for that.
  base::MutexGuard guard(mutex_);
  Grow();

  // Set up the special null entry. This entry must contain nullptr so that
  // empty EmbedderDataSlots represent nullptr.
  static_assert(kNullExternalPointer == 0);
  store(kNullExternalPointer, kNullAddress);
#if defined(__CHERI_PURE_CAPABILITY__)
  // ORing the type information with the pointer invalidates the capability.
  // Therefore, for CHERI the external pointer table stores the tag information
  // seperately in an entry after the pointer. Entries are store in the
  // external pointer table as follows:
  // index = pointer
  // index + 1 = pointer tag
  store(kNullExternalPointerTag, kNullAddress);
#endif
}

void ExternalPointerTable::TearDown() {
  DCHECK(is_initialized());

  GetPlatformVirtualAddressSpace()->FreePages(
      buffer_, kExternalPointerTableReservationSize);
  delete mutex_;

  buffer_ = kNullAddress;
  capacity_ = 0;
  freelist_head_ = 0;
  mutex_ = nullptr;
}

Address ExternalPointerTable::Get(uint32_t index,
                                  ExternalPointerTag tag) const {
#if defined(__CHERI_PURE_CAPABILITY__)
  DCHECK_LT(index, capacity_);
  DCHECK_LT(index + 1, capacity_);
  
  Address entry = load_atomic(index);

  Address entry_tag = load_atomic(index + 1);
  DCHECK(!is_free(entry_tag));

  return (ptraddr_t) (entry_tag & ~tag) == 0 ? entry : (Address) nullptr; 
#else
  DCHECK_LT(index, capacity_);

  Address entry = load_atomic(index);
  DCHECK(!is_free(entry));

 return entry & ~tag;
#endif
}

void ExternalPointerTable::Set(uint32_t index, Address value,
                               ExternalPointerTag tag) {
  DCHECK_NE(kNullExternalPointer, index);
  DCHECK(is_marked(tag));
  DCHECK_EQ(0, value & kExternalPointerTagMask);
#if defined(__CHERI_PURE_CAPABILITY__)
  DCHECK_LT(index, capacity_);
  DCHECK_LT(index + 1, capacity_);

  store_atomic(index, value);
  store_atomic(index + 1, tag);
#else
  DCHECK_LT(index, capacity_);
  store_atomic(index, value | tag);
#endif
}

uint32_t ExternalPointerTable::Allocate() {
  DCHECK(is_initialized());

  base::Atomic32* freelist_head_ptr =
      reinterpret_cast<base::Atomic32*>(&freelist_head_);

  uint32_t index;
  bool success = false;
  while (!success) {
    // This is essentially DCLP (see
    // https://preshing.com/20130930/double-checked-locking-is-fixed-in-cpp11/)
    // and so requires an acquire load as well as a release store in Grow() to
    // prevent reordering of memory accesses, which could for example cause one
    // thread to read a freelist entry before it has been properly initialized.
    uint32_t freelist_head = base::Acquire_Load(freelist_head_ptr);
    if (!freelist_head) {
      // Freelist is empty. Need to take the lock, then attempt to grow the
      // table if no other thread has done it in the meantime.
      base::MutexGuard guard(mutex_);

      // Reload freelist head in case another thread already grew the table.
      freelist_head = base::Relaxed_Load(freelist_head_ptr);

      if (!freelist_head) {
        // Freelist is (still) empty so grow the table.
        freelist_head = Grow();
      }
    }

    DCHECK(freelist_head);
    DCHECK_LT(freelist_head, capacity_);
    index = freelist_head;

    // The next free element is stored in the lower 32 bits of the entry.
    uint32_t new_freelist_head = static_cast<uint32_t>(load_atomic(index));

    uint32_t old_val = base::Relaxed_CompareAndSwap(
        freelist_head_ptr, freelist_head, new_freelist_head);
    success = old_val == freelist_head;
  }

  return index;
}

void ExternalPointerTable::Mark(uint32_t index) {
#if defined(__CHERI_PURE_CAPABILITY__)
  DCHECK_LT(index, capacity_);
  DCHECK_LT(index + 1, capacity_);
  static_assert(sizeof(base::AtomicIntPtr) == sizeof(Address));
#else
  DCHECK_LT(index, capacity_);
  static_assert(sizeof(base::Atomic64) == sizeof(Address));
#endif

#if defined(__CHERI_PURE_CAPABILITY__)
  base::AtomicIntPtr old_val = load_atomic(index + 1);
#else
  base::Atomic64 old_val = load_atomic(index);
#endif
  DCHECK(!is_free(old_val));
  base::Atomic64 new_val = set_mark_bit(old_val);

  // We don't need to perform the CAS in a loop: if the new value is not equal
  // to the old value, then the mutator must've just written a new value into
  // the entry. This in turn must've set the marking bit already (see
  // ExternalPointerTable::Set), so we don't need to do it again.
#if defined(__CHERI_PURE_CAPABILITY__)
  base::AtomicIntPtr* ptr = reinterpret_cast<base::AtomicIntPtr*>(entry_address(index + 1));
#else
  base::Atomic64* ptr = reinterpret_cast<base::Atomic64*>(entry_address(index));
#endif
  base::Atomic64 val = base::Relaxed_CompareAndSwap(ptr, old_val, new_val);
  DCHECK((val == old_val) || is_marked(val));
  USE(val);
}

}  // namespace internal
}  // namespace v8

#endif  // V8_ENABLE_SANDBOX

#endif  // V8_SANDBOX_EXTERNAL_POINTER_TABLE_INL_H_
