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
#pragma once
#include <ostream>
#include <vector>

#include "ast.h"

namespace sscad {
// instructions with the I suffix: uses immediate value
// immediate value: next byte if it is not 0x80,
// next 4 bytes as int32 otherwise.
// Next instruction location depends on the next byte,
// index = current + 2 if next byte is not 0x80,
// index = current + 6 otherwise.
//
// For instructions without immediate values, it is just index + 1.
enum class Instruction : unsigned char {
  // copy and push the i-th local to the top of the stack.
  // note that the i-th parameter of the function is also the i-th local.
  GetI,
  // pop and set the i-th local as the top of the stack.
  SetI,
  // add an constant constant to the top of the stack.
  AddI,
  // jump n bytes relative to the current instruction
  // e.g. if the current instruction is at index 0 and the immediate is 4,
  // the next instruction to be executed is at index 4.
  JumpI,
  // pop the top of the stack, jump n bytes relative current instruction if it
  // is false, and go to the next instruction normally otherwise.
  JumpFalseI,
  // Expects a list and an integer i at the top of the stack.
  // Initially the integer should be -1.
  // When the integer is less than the length of the list,
  // increment the integer by 1 and copy the corresponding element of the list
  // to the top of the stack (without removing anything).
  // When the integer equals to the length of the list,
  // pop the list and the integer from the list, and
  // jump n bytes relative to the current instruction
  Iter,
  // pop and discard the value in the top of the stack
  Pop,
  // duplicate and push the value in the top of the stack
  Dup,
  // unary operation that directly modifies the top of the stack
  // the next char is an enum for the builtin operation
  BuiltinUnaryOp,
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
  // copy and push the i-th global to the top of the stack.
  GetGlobalI,
  // pop and set the i-th global as the top of the stack.
  SetGlobalI,
  // call the function with ID i
  CallI,
  // call the function with ID i
  TailCallI,
  // return the top value of the stack
  Ret,
  // start, step, end, END_OF_STACK
  // pop the last 3 numbers from the stack and push a range value in the
  // stack
  MakeRange,
  // push an empty list to the stack
  MakeList,
  // just for debugging for now
  Echo
};

// clang-format off
enum class BuiltinUnary : unsigned char {
  NOT,
  // vector
  NORM, LEN,
  // range getters
  RBEGIN, RSTEP, REND,
  // numerical
  NEG, SIN, COS, TAN, ASIN, ACOS, ATAN, ABS,
  CEIL, FLOOR, LN, LOG, ROUND, SIGN, SQRT,
};
// clang-format on

void addImm(std::vector<unsigned char> &instructions, int imm);
void addInst(std::vector<unsigned char> &instructions, Instruction i);
void addInst(std::vector<unsigned char> &instructions, Instruction i, int imm);
void addDouble(std::vector<unsigned char> &instructions, double value);
void addBinOp(std::vector<unsigned char> &instructions, BinOp op);
void addUnaryOp(std::vector<unsigned char> &instructions, BuiltinUnary op);

void print(std::ostream &ostream,
           const std::vector<unsigned char> &instructions, bool labels = true);
std::string getInstName(Instruction inst);
}  // namespace sscad
