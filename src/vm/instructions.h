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
#include <cstdint>
namespace sscad {

// immediate value: next byte if it is not 0xFF,
// next 4 bytes as int32 otherwise.
// Next instruction location depends on the next byte,
// index = current + 2 if next byte is not 0xFF,
// index = current + 6 otherwise.
//
// For instructions without immediate values, it is just index + 1.
enum Instruction : char {
  // add an constant constant to the top of the stack.
  AddI,
  // negate the top of the stack.
  Negation,
  // logical not the top of the stack,
  // performs conversion to boolean if necessary.
  LogicalNot,
  // rhs = stack.pop(), stack.top() = stack.top() OP rhs.
  // the next char is the binary operation.
  BinaryOp,
  // push a constant double to the top of the stack.
  // the next 8 bytes in machine endian represents the double.
  // next instruction index: current + 9
  ConstNum,
  // push an undef value to the top of the stack if the next byte is 2,
  // true if the next byte is 1, and false if the next byte is 0.
  // next instruction index: current + 2
  ConstMisc,
  // copy and push the i-th local to the top of the stack.
  // note that the i-th parameter of the function is also the i-th local.
  GetI,
  // pop and set the i-th local as the top of the stack.
  SetI,
  // copy and push the i-th global to the top of the stack.
  GetGlobalI,
  // pop and set the i-th global as the top of the stack.
  SetGlobalI,
  // copy and push the i-th local in the parent scope to the top of the stack.
  // used for module children statements
  GetParentI,
  // pop and discard the value in the top of the stack
  Pop,
  // duplicate and push the value in the top of the stack
  Dup,
  // jump n bytes relative to the current instruction
  // e.g. if the current instruction is at index 0 and the immediate is 4,
  // the next instruction to be executed is at index 4.
  JumpI,
  // pop the top of the stack, jump n bytes relative current instruction if it
  // is zero, and go to the next instruction normally otherwise.
  JumpZeroI,
  // return the top value of the stack
  Ret,
  // call the function with ID i
  CallI,
  // call the module with ID i
  // note that this is different from function call due to how module stack is
  // setup (for GetParentI)
  CallModuleI,
};
}
