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
#include "instructions.h"

#include <cstring>

namespace sscad {
void addImm(std::vector<unsigned char> &instructions, int imm) {
  if (imm > -128 && imm <= 127)
    instructions.push_back(static_cast<char>(imm));
  else {
    instructions.push_back(0x80);
    instructions.resize(instructions.size() + 4);
    memcpy(instructions.data() + instructions.size() - 4, &imm, sizeof(int));
  }
}
void addInst(std::vector<unsigned char> &instructions, Instruction i) {
  instructions.push_back(static_cast<unsigned char>(i));
}
void addInst(std::vector<unsigned char> &instructions, Instruction i, int imm) {
  instructions.push_back(static_cast<unsigned char>(i));
  addImm(instructions, imm);
}
void addDouble(std::vector<unsigned char> &instructions, double value) {
  instructions.push_back(static_cast<unsigned char>(Instruction::ConstNum));
  instructions.resize(instructions.size() + 8);
  memcpy(instructions.data() + instructions.size() - 8, &value, sizeof(double));
}
void addBinOp(std::vector<unsigned char> &instructions, BinOp op) {
  addInst(instructions, Instruction::BinaryOp);
  instructions.push_back(static_cast<unsigned char>(op));
}
void addUnaryOp(std::vector<unsigned char> &instructions, BuiltinUnary op) {
  addInst(instructions, Instruction::BuiltinUnaryOp);
  instructions.push_back(static_cast<unsigned char>(op));
}
}  // namespace sscad
