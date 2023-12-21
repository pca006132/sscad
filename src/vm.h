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
/**
 * Value tag representing the type for certain values.
 * The value itself is stored *tagless* in the memory, 64-bit each. When stored
 * on the stack, there is a tag stack alongside with the value stack. When
 * stored in a vector, the vector contains a tag list before the values.
 */
enum ValueTag : char {
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by a null-terminated utf-8 C string.
  // Aligned to 8-bytes.
  String = 0x0,
  // The normal heterogeneous vector in OpenSCAD.
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by a 32-bit length counter (N), N 64-bit values,
  // and N 8-bit tags at the end.
  // Aligned to 8-bytes.
  Vector = 0x1,
  // Specialized 1D/2D number vector. For the 2D case it must be a valid matrix,
  // i.e. all inner vectors should have the same length. The idea is to keep
  // arrays as arrays if possible, and degenerate to a vector when user tries to
  // create something that violates the assumptions of an array, e.g.
  // non-uniform sizes or adding strings.
  //
  // A 64-bit pointer to a continuous buffer, containing a 32-bit integer
  // reference count, followed by two 32-bit integer dimension (row and column,
  // 1D vector has column length 0), one 32-bit padding, and a list of 64-bit
  // floating point numbers in row major order.
  // Aligned to 8-bytes.
  Array = 0x2,
  // Geometry objects returned by modules.
  //
  // A 64-bit pointer to a struct, with a 32-bit integer reference count,
  // 32-bit padding and a 64-bit pointer to the underlying geometry object
  // (provided by the user).
  // Aligned to 8-bytes.
  Geometry = 0x3,
  // Range iterator
  //
  // A 64-bit pointer to a struct, with a 32-bit integer reference count, 32-bit
  // padding, and three 64-bit floating point values representing start, end and
  // step.
  Range = 0x4,
  // Just a 64-bit floating point number, nothing special.
  Number = 0x5,
  // Just undef, the value has no meaning.
  Undef = 0x6,
  // A 64-bit value to represent boolean, with true = 1 and false = 0.
  Boolean = 0x7,
};

}  // namespace sscad
