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
namespace sscad {
// TODO: Use structs for things other than Array, we don't need that good
// performance for other stuff.
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
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by a null-terminated utf-8 C string.
  String = 0x0,
  // The normal heterogeneous vector in OpenSCAD.
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by a 32-bit length counter (N), a 32-bit capacity
  // counter (C), N 8-bit tags, some padding and N 64-bit values.
  // The padding is calculated such that the 64-bit values are aligned to
  // 8-bytes and larger than C - N.
  // The total capacity is doubled when reallocated, this provides amortized
  // constant-time insertion performance when the usage is unique.
  // Note that capacity is the number of values inside, not the total size of
  // the buffer.
  Vector,
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
  Array,
  // Geometry objects returned by modules.
  //
  // A 64-bit pointer to a struct, with a 32-bit integer reference count,
  // 32-bit padding and a 64-bit pointer to the underlying geometry object
  // (provided by the user).
  // The pointer can be a nullptr (0) if the module is empty. This avoids asking
  // the user for an empty object pointer.
  Geometry,
  // Range iterator
  //
  // A 64-bit pointer to a struct, with a 32-bit integer reference count, 32-bit
  // padding, and three 64-bit floating point values representing start, end and
  // step.
  Range,

  // ============================================================================
  // Just a 64-bit floating point number, nothing special.
  Number = 0x10,
  // Just undef, the value has no meaning.
  Undef,
  // A 64-bit value to represent boolean, with true = 1 and false = 0.
  Boolean,
};

constexpr bool isAllocated(ValueTag tag) { return (tag & 0xF0) == 0; }

}  // namespace sscad
