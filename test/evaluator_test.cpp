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

#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <thread>

#include "ast.h"

using namespace sscad;
using namespace std::chrono_literals;

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
  addDouble(list1, 10000000000);
  addDouble(list1, 12.34);
  int looppc = list1.size();
  addInst(list1, Instruction::AddI, 1);
  addInst(list1, Instruction::AddI, 200);
  addInst(list1, Instruction::Dup);
  addInst(list1, Instruction::GetI, 0);
  addBinOp(list1, BinOp::LE);
  int currentIndex = list1.size();
  addInst(list1, Instruction::JumpTrueI, looppc - currentIndex);
  addInst(list1, Instruction::Echo);
  addInst(list1, Instruction::Ret);

  /**
   * function foo(a, b) = a <= 0 ? b : foo(a - 1, b + 2);
   * function entry() = foo(100000, 0);
   */
  constexpr bool useTailcall = true;
  std::vector<unsigned char> foo;
  addInst(foo, Instruction::GetI, 0);
  addInst(foo, Instruction::Dup, 0);
  addBinOp(foo, BinOp::LE);
  addInst(foo, Instruction::JumpTrueI, useTailcall ? 10 : 11);
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
   * for (int i = 0; i < 100; i++) {
   *   int sum = 0;
   *   for (int j = 0; j < 100000; j++)
   *     sum += 2;
   * }
   */
  constexpr bool doEcho = true;
  std::vector<unsigned char> pureloop;
  addDouble(pureloop, 100);
  addDouble(pureloop, 100000);
  addDouble(pureloop, 0);  // local 2
  int pureloopOuter = pureloop.size();
  addInst(pureloop, Instruction::GetI, 2);
  addInst(pureloop, Instruction::Dup);
  addInst(pureloop, Instruction::GetI, 0);
  addBinOp(pureloop, BinOp::LT);
  addInst(pureloop, Instruction::JumpTrueI, 3);
  addInst(pureloop, Instruction::Ret);
  addInst(pureloop, Instruction::AddI, 1);
  addInst(pureloop, Instruction::SetI, 2);
  addDouble(pureloop, 0);
  int pureloopInner = pureloop.size();
  addInst(pureloop, Instruction::Dup);
  addInst(pureloop, Instruction::GetI, 1);
  addBinOp(pureloop, BinOp::LT);
  addInst(pureloop, Instruction::JumpTrueI, 4 + (doEcho?1:0));
  if (doEcho)
    addInst(pureloop, Instruction::Echo);
  addInst(pureloop, Instruction::JumpI, pureloopOuter - pureloop.size());
  addInst(pureloop, Instruction::AddI, 2);
  addInst(pureloop, Instruction::JumpI, pureloopInner - pureloop.size());

  Evaluator evaluator(
      &std::cout,
      {FunctionEntry{list1, 0, false}, FunctionEntry{foo, 2, false},
       FunctionEntry{entry, 0, false}, FunctionEntry{pureloop, 0, false}},
      {}, {});
  // auto future = std::async(std::launch::async, [&] {
  //   std::this_thread::sleep_for(3s);
  //   std::cout << "stopping" << std::endl;
  //   evaluator.stop();
  // });
  // evaluator.eval(0);
  std::cout << "------------" << std::endl;
  evaluator.eval(3);
  // for (int i = 0; i < 100; i++) evaluator.eval(2);
  return 0;
}
