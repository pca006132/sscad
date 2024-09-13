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

#include "vm/evaluator.h"

#include <iostream>

#include "ast.h"
#include "vm/instructions.h"

using namespace sscad;

int main() {
  /**
   * Basically:
   *
   * double d = 12.34;
   * do {
   *   // testing different immediate sizes
   *   d += 1;
   *   d += 200;
   * } while (d <= 10000000000);
   * printf(d);
   * return d;
   */
  std::vector<unsigned char> list1;
  addDouble(list1, 10000);
  addDouble(list1, 0);
  int looppc = list1.size();
  addInst(list1, Instruction::AddI, +1);
  // addInst(list1, Instruction::AddI, 200);
  addInst(list1, Instruction::Dup);
  addInst(list1, Instruction::GetI, 0);
  addBinOp(list1, BinOp::GE);
  addInst(list1, Instruction::JumpFalseI, looppc - list1.size());
  // addInst(list1, Instruction::Echo);
  addInst(list1, Instruction::Ret);

  /**
   * function foo(a, b) = a <= 0 ? b : foo(a - 1, b + 2);
   * function entry() = foo(100000, 0);
   */
  constexpr bool useTailcall = true;
  std::vector<unsigned char> foo;
  addInst(foo, Instruction::GetI, 0);
  addInst(foo, Instruction::Dup);
  addDouble(foo, 0);
  addBinOp(foo, BinOp::GT);
  addInst(foo, Instruction::JumpFalseI, useTailcall ? 10 : 11);
  addInst(foo, Instruction::AddI, -1);
  addInst(foo, Instruction::GetI, 1);
  addInst(foo, Instruction::AddI, 2);
  addInst(foo, useTailcall ? Instruction::TailCallI : Instruction::CallI, 1);
  if (!useTailcall) addInst(foo, Instruction::Ret);

  addInst(foo, Instruction::GetI, 1);
  addInst(foo, Instruction::Ret);

  std::vector<unsigned char> entry;
  addDouble(entry, 100000);
  addDouble(entry, 0);
  addInst(entry, Instruction::CallI, 1);
  // uncomment to print stuff
  // addInst(entry, Instruction::Echo);
  addInst(entry, Instruction::Ret);

  /**
   * for (int i = 0; i < 1000; i++) {
   *   int sum = 0;
   *   for (int j = 0; j < 100000; j++)
   *     sum += 2;
   * }
   */
  constexpr bool doEcho = false;
  std::vector<unsigned char> pureloop;
  addDouble(pureloop, 100'000'000);
  addDouble(pureloop, 0);
  int pureloop_l1 = pureloop.size();
  addInst(pureloop, Instruction::Dup);
  addInst(pureloop, Instruction::GetI, 0);
  addBinOp(pureloop, BinOp::GE);
  addInst(pureloop, Instruction::JumpFalseI, 3);
  addInst(pureloop, Instruction::Ret);
  addInst(pureloop, Instruction::AddI, 1);
  addInst(pureloop, Instruction::JumpI, pureloop_l1 - pureloop.size());
  // print(std::cout, pureloop);

  // addDouble(pureloop, 100000);
  // addDouble(pureloop, 0);  // local 2
  // int pureloopOuter = pureloop.size();
  // addInst(pureloop, Instruction::GetI, 2);
  // addInst(pureloop, Instruction::Dup);
  // addInst(pureloop, Instruction::GetI, 0);
  // addBinOp(pureloop, BinOp::GE);
  // addInst(pureloop, Instruction::JumpFalseI, 3);
  // addInst(pureloop, Instruction::Ret);
  // addInst(pureloop, Instruction::AddI, 1);
  // addInst(pureloop, Instruction::SetI, 2);
  // addDouble(pureloop, 0);
  // int pureloopInner = pureloop.size();
  // addInst(pureloop, Instruction::Dup);
  // addInst(pureloop, Instruction::GetI, 1);
  // addBinOp(pureloop, BinOp::GE);
  // addInst(pureloop, Instruction::JumpFalseI, 4 + (doEcho ? 1 : 0));
  // if (doEcho) addInst(pureloop, Instruction::Echo);
  // addInst(pureloop, Instruction::JumpI, pureloopOuter - pureloop.size());
  // addInst(pureloop, Instruction::AddI, 1);
  // addInst(pureloop, Instruction::JumpI, pureloopInner - pureloop.size());

  // print(std::cout, pureloop);

  Evaluator evaluator(
      &std::cout,
      {FunctionEntry{list1, 0, false}, FunctionEntry{foo, 2, false},
       FunctionEntry{entry, 0, false}, FunctionEntry{pureloop, 0, false}},
      {}, {});
  // for (int i = 0; i < 10000; i++) evaluator.eval(0);
  // std::cout << "------------" << std::endl;
  // for (int i = 0; i < 100; i++) evaluator.eval(2);
  evaluator.eval(3);
  return 0;
}
