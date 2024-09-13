/**
 * Copyright 2023 The sscad Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <memory>
#include <string>
#include <vector>

namespace sscad {
// handle for geometry object
// note that 0 represents the empty object
using SGeometry = long;

/**
 * Value tag representing the type for certain values.
 * The value itself is stored *tagless* in the memory, 64-bit each. When stored
 * on the stack, there is a tag stack alongside with the value stack. When
 * stored in a vector, the vector contains a tag list before the values. Values
 * and buffers are always aligned to 8-bytes.
 *
 * Normally, instructions that modifies allocated types will copy the underlying
 * buffer. If the frontend can deduce that the old value is no longer used in
 * the current scope (i.e. not stored in a local variable), it can use special
 * variant of the instruction to perform direct mutation. The special
 * instruction will check the reference count and perform direct mutation when
 * it is 1 (unique reference globally), or copy the buffer first if the
 * reference is not unique.
 */
enum ValueTag : char {
  STRING = 0x0,
  // The normal heterogeneous vector in OpenSCAD.
  VECTOR,
  // Range iterator
  RANGE,
  // Specialized 1D/2D number vector. For the 2D case it must be a valid matrix,
  // i.e. all inner vectors should have the same length. The idea is to keep
  // arrays as arrays if possible, and degenerate to a vector when user tries to
  // create something that violates the assumptions of an array, e.g.
  // non-uniform sizes or adding strings.
  //
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by two 32-bit integer dimension (row and column,
  // 1D vector has column length 0), one 32-bit total capacity, and a list of
  // 64-bit floating point numbers in row major order.
  // The total capacity is doubled when reallocated, this provides amortized
  // constant-time insertion performance when the usage is unique.
  //
  // TODO: Unimplemented for now
  // ARRAY,
  // ============================================================================
  // Just a 64-bit floating point number, nothing special.
  NUMBER = 0x10,
  // Handle for geometry objects returned by modules.
  // Note that we never need to destruct this.
  GEOMETRY,
  // Just undef, the value has no meaning.
  UNDEF,
  // A 64-bit value to represent boolean, with true = 1 and false = 0.
  BOOLEAN,
};

constexpr bool isAllocated(ValueTag tag) { return tag < 0x10; }

struct SVector;
struct SRange;

/**
 * We are using untagged union here, so we have to handle the object destruction
 * by ourselves.
 * This is necessary because std::variant is huge, 24 bytes (with
 * std::shared_ptr) compared to the untagged union here (8 bytes).
 * We treat all the allocated things as unique pointers.
 */
union SValue {
  double number;
  SGeometry geometry;
  bool cond;
  std::string* s;
  // if vec equals NULL, the list is an empty list
  SVector* vec;
  SRange* range;
  // unsigned char* array;
};

struct ValuePair {
  ValueTag tag;
  SValue value;
  constexpr ValuePair(ValueTag tag, SValue value) : tag(tag), value(value) {}
  constexpr ValuePair(double number)
      : tag(ValueTag::NUMBER), value(SValue{.number = number}) {}
  constexpr ValuePair(size_t number)
      : tag(ValueTag::NUMBER),
        value(SValue{.number = static_cast<double>(number)}) {}
  constexpr ValuePair(bool cond)
      : tag(ValueTag::BOOLEAN), value(SValue{.cond = cond}) {}
  constexpr static ValuePair undef() {
    return ValuePair(ValueTag::UNDEF, SValue{});
  }

  bool operator==(ValuePair rhs) const;
  bool operator!=(ValuePair rhs) const { return !(*this == rhs); }
};

struct SVector {
  std::shared_ptr<std::vector<ValuePair>> values;
};

struct SRange {
  double begin;
  double step;
  double end;

  constexpr bool operator==(const SRange& other) {
    return begin == other.begin && end == other.end && step == other.step;
  }
};

}  // namespace sscad
