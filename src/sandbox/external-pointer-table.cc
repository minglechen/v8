// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/sandbox/external-pointer-table.h"

#include <algorithm>

#include "src/execution/isolate.h"
#include "src/logging/counters.h"
#include "src/sandbox/external-pointer-table-inl.h"

#ifdef V8_ENABLE_SANDBOX

namespace v8 {
namespace internal {

static_assert(sizeof(ExternalPointerTable) == ExternalPointerTable::kSize);

// static
uint32_t ExternalPointerTable::AllocateEntry(ExternalPointerTable* table) {
  return table->Allocate();
}

uint32_t ExternalPointerTable::Sweep(Isolate* isolate) {
  // Sweep top to bottom and rebuild the freelist from newly dead and
  // previously freed entries. This way, the freelist ends up sorted by index,
  // which helps defragment the table. This method must run either on the
  // mutator thread or while the mutator is stopped. Also clear marking bits on
  // live entries.
  // TODO(v8:10391, saelo) could also shrink the table using DecommitPages() if
  // elements at the end are free. This might require some form of compaction.
  uint32_t freelist_size = 0;
  uint32_t current_freelist_head = 0;

  // Skip the special null entry.
  DCHECK_GE(capacity_, 1);
#if defined(__CHERI_PURE_CAPABILITY__)
  for (uint32_t i = capacity_ - 2; i > 0; i=i-2) {
#else
  for (uint32_t i = capacity_ - 1; i > 0; i--) {
#endif
    // No other threads are active during sweep, so there is no need to use
    // atomic operations here.
#if defined(__CHERI_PURE_CAPABILITY__)
    // Load the tag stored along with the external pointer
    Address entry = load(i + 1);
#else
    Address entry = load(i);
#endif
    if (!is_marked(entry)) {
#if defined(__CHERI_PURE_CAPABILITY__)
      store(i, current_freelist_head);
      store(i + 1, kExternalPointerFreeEntryTag);
#else
      store(i, make_freelist_entry(current_freelist_head));
#endif
      current_freelist_head = i;
      freelist_size++;
    } else {
#if defined(__CHERI_PURE_CAPABILITY__)
      store(i + 1, clear_mark_bit(entry));
#else
      store(i, clear_mark_bit(entry));
#endif
    }
  }

  freelist_head_ = current_freelist_head;

#if defined(__CHERI_PURE_CAPABILITY__)
  uint32_t num_active_entries = capacity_ / 2 - freelist_size;
#else
  uint32_t num_active_entries = capacity_ - freelist_size;
#endif
  isolate->counters()->sandboxed_external_pointers_count()->AddSample(
      num_active_entries);
  return num_active_entries;
}

uint32_t ExternalPointerTable::Grow() {
  // Freelist should be empty.
  DCHECK_EQ(0, freelist_head_);
  // Mutex must be held when calling this method.
  mutex_->AssertHeld();

  // Grow the table by one block.
  uint32_t old_capacity = capacity_;
  uint32_t new_capacity = old_capacity + kEntriesPerBlock;
  CHECK_LE(new_capacity, kMaxSandboxedExternalPointers);

  // Failure likely means OOM. TODO(saelo) handle this.
  VirtualAddressSpace* root_space = GetPlatformVirtualAddressSpace();
  DCHECK(IsAligned(kBlockSize, root_space->page_size()));
  CHECK(root_space->SetPagePermissions(buffer_ + old_capacity * sizeof(Address),
                                       kBlockSize,
                                       PagePermissions::kReadWrite));
  capacity_ = new_capacity;

  // Build freelist bottom to top, which might be more cache friendly.
#if defined(__CHERI_PURE_CAPABILITY__)
  uint32_t start = std::max<uint32_t>(old_capacity, 2);  // Skip entry zero
  uint32_t last = new_capacity - 2;
  for (uint32_t i = start; i < last; i+=2) {
    store(i, i + 2);
    store(i + 1, kExternalPointerFreeEntryTag);
#else
  uint32_t start = std::max<uint32_t>(old_capacity, 1);  // Skip entry zero
  uint32_t last = new_capacity - 1;
  for (uint32_t i = start; i < last; i++) {
    store(i, make_freelist_entry(i + 1));
#endif
  }
#if defined(__CHERI_PURE_CAPABILITY__)
  store(last, 0);
  store(last + 1, kExternalPointerFreeEntryTag);
#else
  store(last, make_freelist_entry(0));
#endif

  // This must be a release store to prevent reordering of the preceeding
  // stores to the freelist from being reordered past this store. See
  // Allocate() for more details.
  base::Release_Store(reinterpret_cast<base::Atomic32*>(&freelist_head_),
                      start);
  return start;
}

}  // namespace internal
}  // namespace v8

#endif  // V8_ENABLE_SANDBOX
