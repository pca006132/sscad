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

void addInst(std::vector<unsigned char> &instructions, Instruction i) {
  instructions.push_back(static_cast<unsigned char>(i));
}
void addDouble(std::vector<unsigned char> &instructions, double value) {
  instructions.resize(instructions.size() + 8);
  memcpy(instructions.data() + instructions.size() - 8, &value, sizeof(double));
}
void addImm(std::vector<unsigned char> &instructions, short imm) {
  if (imm > -128 && imm <= 127)
    instructions.push_back(static_cast<unsigned char>(imm));
  else {
    instructions.push_back(0xFF);
    instructions.resize(instructions.size() + 4);
    memcpy(instructions.data() + instructions.size() - 4, &imm, sizeof(short));
  }
}
void addBinOp(std::vector<unsigned char> &instructions, BinOp op) {
  addInst(instructions, Instruction::BinaryOp);
  instructions.push_back(static_cast<unsigned char>(op));
}

int main() {
  std::vector<unsigned char> list1;
  addInst(list1, Instruction::ConstNum);
  addDouble(list1, 10000000000);
  addInst(list1, Instruction::ConstNum);
  addDouble(list1, 12.34);
  int looppc = list1.size();
  addInst(list1, Instruction::AddI);
  addImm(list1, 1);
  addInst(list1, Instruction::AddI);
  addImm(list1, 200);
  addInst(list1, Instruction::Dup);
  addInst(list1, Instruction::GetI);
  addImm(list1, 0);
  addBinOp(list1, BinOp::LE);
  int currentIndex = list1.size();
  addInst(list1, Instruction::JumpTrueI);
  addImm(list1, looppc - currentIndex);

  addInst(list1, Instruction::Echo);
  addInst(list1, Instruction::Ret);

  Evaluator evaluator(&std::cout, {FunctionEntry{list1, 0, false}}, {}, {});
  // auto future = std::async(std::launch::async, [&] {
  //   std::this_thread::sleep_for(3s);
  //   std::cout << "stopping" << std::endl;
  //   evaluator.stop();
  // });
  evaluator.eval(0);
  return 0;
}
