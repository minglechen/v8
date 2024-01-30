// Copyright 2021 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(__CHERI_PURE_CAPABILITY__)
#include "src/common/ptr-compr-inl.h"
#endif // defined(__CHERI_PURE_CAPABILITY__)

#include "src/objects/object-type.h"

#include "src/objects/objects-inl.h"
#include "src/objects/smi.h"
#include "src/objects/string-inl.h"

namespace v8 {
namespace internal {

Address CheckObjectType(Address raw_value, Address raw_type,
                        Address raw_location) {
#ifdef DEBUG
  Object value(raw_value);
  Smi type(raw_type);
  String location = String::cast(Object(raw_location));
  const char* expected;
#if defined(__CHERI_PURE_CAPABILITY__)
  // TODO(gcjenkinson): Is##Name performs arbitrary pointer manipulation
  // to work back from the Object to the Isolate instance (through
  // the MemoryChunk instance). This results in invalidated capabilities or
  // an out-of-bounds access protection error.
  return Smi::FromInt(0).ptr();
#else // !defined(__CHERI_PURE_CAPABILITY)
  switch (static_cast<ObjectType>(type.value())) {
#define TYPE_CASE(Name)                                 \
  case ObjectType::k##Name:                             \
    if (value.Is##Name()) return Smi::FromInt(0).ptr(); \
    expected = #Name;                                   \
    break;
#define TYPE_STRUCT_CASE(NAME, Name, name)              \
  case ObjectType::k##Name:                             \
    if (value.Is##Name()) return Smi::FromInt(0).ptr(); \
    expected = #Name;                                   \
    break;

    TYPE_CASE(Object)
    TYPE_CASE(Smi)
    TYPE_CASE(TaggedIndex)
    TYPE_CASE(HeapObject)
    OBJECT_TYPE_LIST(TYPE_CASE)
    HEAP_OBJECT_TYPE_LIST(TYPE_CASE)
    STRUCT_LIST(TYPE_STRUCT_CASE)
#undef TYPE_CASE
#undef TYPE_STRUCT_CASE
  }
#endif // !defined(__CHERI_PURE_CAPABILITY)
  std::stringstream value_description;
  value.Print(value_description);
  FATAL(
      "Type cast failed in %s\n"
      "  Expected %s but found %s",
      location.ToAsciiArray(), expected, value_description.str().c_str());
#else
  UNREACHABLE();
#endif
}

}  // namespace internal
}  // namespace v8
