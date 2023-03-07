// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_COMMON_PTR_COMPR_INL_H_
#define V8_COMMON_PTR_COMPR_INL_H_

#include "include/v8-internal.h"
#include "src/execution/isolate.h"
#include "src/execution/local-isolate-inl.h"

namespace v8 {
namespace internal {

#ifdef V8_COMPRESS_POINTERS

#ifdef V8_EXTERNAL_CODE_SPACE

//
// ExternalCodeCompressionScheme
//

// static
Address ExternalCodeCompressionScheme::PrepareCageBaseAddress(
    Address on_heap_addr) {
  return RoundDown<kMinExpectedOSPageSize>(on_heap_addr);
}

// static
Address ExternalCodeCompressionScheme::GetPtrComprCageBaseAddress(
    PtrComprCageBase cage_base) {
  Address base = cage_base.address();
  base = reinterpret_cast<Address>(
      V8_ASSUME_ALIGNED(reinterpret_cast<void*>(base), kMinExpectedOSPageSize));
  return base;
}

#ifdef V8_COMPRESS_POINTERS_IN_SHARED_CAGE

// static
void ExternalCodeCompressionScheme::InitBase(Address base) {
  CHECK_EQ(base, PrepareCageBaseAddress(base));
  base_ = base;
}

// static
Address ExternalCodeCompressionScheme::base() {
  return reinterpret_cast<Address>(V8_ASSUME_ALIGNED(
      reinterpret_cast<void*>(base_), kMinExpectedOSPageSize));
}
#endif  // V8_COMPRESS_POINTERS_IN_SHARED_CAGE

// static
Tagged_t ExternalCodeCompressionScheme::CompressTagged(Address tagged) {
  return static_cast<Tagged_t>(static_cast<uint32_t>(tagged));
}

// static
Address ExternalCodeCompressionScheme::DecompressTaggedSigned(
    Tagged_t raw_value) {
  // For runtime code the upper 32-bits of the Smi value do not matter.
  return static_cast<Address>(raw_value);
}

// static
template <typename TOnHeapAddress>
Address ExternalCodeCompressionScheme::DecompressTaggedPointer(
    TOnHeapAddress on_heap_addr, Tagged_t raw_value) {
#if defined(V8_COMPRESS_POINTERS_IN_SHARED_CAGE) && \
    !defined(V8_COMPRESS_POINTERS_DONT_USE_GLOBAL_BASE)
  Address cage_base = base();
#else
  Address cage_base = GetPtrComprCageBaseAddress(on_heap_addr);
#endif
  Address diff = static_cast<Address>(static_cast<uint32_t>(raw_value)) -
                 static_cast<Address>(static_cast<uint32_t>(cage_base));
  // The cage base value was chosen such that it's less or equal than any
  // pointer in the cage, thus if we got a negative diff then it means that
  // the decompressed value is off by 4GB.
  if (static_cast<intptr_t>(diff) < 0) {
    diff += size_t{4} * GB;
  }
  DCHECK(is_uint32(diff));
  Address result = cage_base + diff;
  DCHECK_EQ(static_cast<uint32_t>(result), raw_value);
  return result;
}

// static
template <typename TOnHeapAddress>
Address ExternalCodeCompressionScheme::DecompressTaggedAny(
    TOnHeapAddress on_heap_addr, Tagged_t raw_value) {
  if (HAS_SMI_TAG(raw_value)) return DecompressTaggedSigned(raw_value);
  if (raw_value == kClearedWeakHeapObjectLower32) return raw_value;
  return DecompressTaggedPointer(on_heap_addr, raw_value);
}

#endif  // V8_EXTERNAL_CODE_SPACE

//
// Misc functions.
//

PtrComprCageBase::PtrComprCageBase(const Isolate* isolate)
    : address_(isolate->cage_base()) {}
PtrComprCageBase::PtrComprCageBase(const LocalIsolate* isolate)
    : address_(isolate->cage_base()) {}

Address PtrComprCageBase::address() const {
  Address ret = address_;
  ret = reinterpret_cast<Address>(V8_ASSUME_ALIGNED(
      reinterpret_cast<void*>(ret), kPtrComprCageBaseAlignment));
  return ret;
}

// Compresses full-pointer representation of a tagged value to on-heap
// representation.
V8_INLINE Tagged_t CompressTagged(Address tagged) {
  return static_cast<Tagged_t>(static_cast<uint32_t>(tagged));
}

V8_INLINE constexpr Address GetPtrComprCageBaseAddress(Address on_heap_addr) {
  return RoundDown<kPtrComprCageBaseAlignment>(on_heap_addr);
}

V8_INLINE Address GetPtrComprCageBaseAddress(PtrComprCageBase cage_base) {
  return cage_base.address();
}

V8_INLINE constexpr PtrComprCageBase GetPtrComprCageBaseFromOnHeapAddress(
    Address address) {
  return PtrComprCageBase(GetPtrComprCageBaseAddress(address));
}

// Decompresses smi value.
V8_INLINE Address DecompressTaggedSigned(Tagged_t raw_value) {
  // For runtime code the upper 32-bits of the Smi value do not matter.
  return static_cast<Address>(raw_value);
}

// Decompresses weak or strong heap object pointer or forwarding pointer,
// preserving both weak- and smi- tags.
template <typename TOnHeapAddress>
V8_INLINE Address DecompressTaggedPointer(TOnHeapAddress on_heap_addr,
                                          Tagged_t raw_value) {
  return GetPtrComprCageBaseAddress(on_heap_addr) +
         static_cast<Address>(raw_value);
}

// Decompresses any tagged value, preserving both weak- and smi- tags.
template <typename TOnHeapAddress>
V8_INLINE Address DecompressTaggedAny(TOnHeapAddress on_heap_addr,
                                      Tagged_t raw_value) {
  return DecompressTaggedPointer(on_heap_addr, raw_value);
}

#else
V8_INLINE Tagged_t CompressTagged(Address tagged) { UNREACHABLE(); }

V8_INLINE constexpr PtrComprCageBase GetPtrComprCageBaseFromOnHeapAddress(
    Address address) {
  return PtrComprCageBase();
}

V8_INLINE Address DecompressTaggedSigned(Tagged_t raw_value) { UNREACHABLE(); }

template <typename TOnHeapAddress>
V8_INLINE Address DecompressTaggedPointer(TOnHeapAddress on_heap_addr,
                                          Tagged_t raw_value) {
  UNREACHABLE();
}

template <typename TOnHeapAddress>
V8_INLINE Address DecompressTaggedAny(TOnHeapAddress on_heap_addr,
                                      Tagged_t raw_value) {
  UNREACHABLE();
}

V8_INLINE Address GetPtrComprCageBaseAddress(Address on_heap_addr) {
  UNREACHABLE();
}

#endif  // V8_COMPRESS_POINTERS

V8_INLINE PtrComprCageBase GetPtrComprCageBase(HeapObject object) {
  return GetPtrComprCageBaseFromOnHeapAddress(object.ptr());
}

}  // namespace internal
}  // namespace v8

#endif  // V8_COMMON_PTR_COMPR_INL_H_
