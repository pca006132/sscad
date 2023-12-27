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
#include <set>

#include "utils/ast_printer.h"

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

static std::pair<int, int> getImmediate(
    const std::vector<unsigned char> &instructions, int currentPC) {
  if (currentPC + 1 >= instructions.size())
    throw std::runtime_error("invalid bytecode");
  if (instructions[currentPC + 1] != 0x80) {
    const char *p =
        reinterpret_cast<const char *>(instructions.data() + currentPC + 1);
    return std::make_pair(static_cast<int>(*p), 2);
  }
  if (currentPC + 5 >= instructions.size())
    throw std::runtime_error("invalid bytecode");
  int p;
  memcpy(&p, instructions.data() + currentPC + 2, sizeof(int));
  return std::make_pair(p, 6);
}

std::string getInstName(Instruction inst) {
  switch (inst) {
    case Instruction::AddI:
      return "AddI";
    case Instruction::GetI:
      return "GetI";
    case Instruction::GetParentI:
      return "GetParentI";
    case Instruction::SetI:
      return "SetI";
    case Instruction::GetGlobalI:
      return "GetGlobalI";
    case Instruction::SetGlobalI:
      return "SetGlobalI";
    case Instruction::JumpI:
      return "JumpI";
    case Instruction::JumpFalseI:
      return "JumpFalseI";
    case Instruction::CallI:
      return "CallI";
    case Instruction::TailCallI:
      return "TailCallI";
    case Instruction::BuiltinUnaryOp:
      return "BuiltinUnaryOp";
    case Instruction::BinaryOp:
      return "BinaryOp";
    case Instruction::ConstNum:
      return "ConstNum";
    case Instruction::ConstMisc:
      return "ConstMisc";
    case Instruction::Pop:
      return "Pop";
    case Instruction::Dup:
      return "Dup";
    case Instruction::Ret:
      return "Ret";
    case Instruction::Echo:
      return "Echo";
  }
}

std::ostream &operator<<(std::ostream &ostream, BuiltinUnary unary) {
  switch (unary) {
    case BuiltinUnary::NOT:
      ostream << "not";
      break;
    case BuiltinUnary::NEG:
      ostream << "neg";
      break;
    case BuiltinUnary::SIN:
      ostream << "sin";
      break;
    case BuiltinUnary::COS:
      ostream << "cos";
      break;
    case BuiltinUnary::TAN:
      ostream << "tan";
      break;
    case BuiltinUnary::ASIN:
      ostream << "asin";
      break;
    case BuiltinUnary::ACOS:
      ostream << "acos";
      break;
    case BuiltinUnary::ATAN:
      ostream << "atan";
      break;
    case BuiltinUnary::ABS:
      ostream << "abs";
      break;
    case BuiltinUnary::CEIL:
      ostream << "ceil";
      break;
    case BuiltinUnary::FLOOR:
      ostream << "floor";
      break;
    case BuiltinUnary::LN:
      ostream << "ln";
      break;
    case BuiltinUnary::LOG:
      ostream << "log";
      break;
    case BuiltinUnary::NORM:
      ostream << "norm";
      break;
    case BuiltinUnary::ROUND:
      ostream << "round";
      break;
    case BuiltinUnary::SIGN:
      ostream << "sign";
      break;
    case BuiltinUnary::SQRT:
      ostream << "sqrt";
      break;
  }
  return ostream;
}

void print(std::ostream &ostream, std::vector<unsigned char> &instructions,
           bool labels) {
  std::set<int> labelIndices;
  int pc = 0;
  if (labels) {
    // find all jump statements and identify label positions
    while (pc < instructions.size()) {
      Instruction inst = static_cast<Instruction>(instructions[pc]);
      switch (inst) {
        case Instruction::AddI:
        case Instruction::GetI:
        case Instruction::SetI:
        case Instruction::GetGlobalI:
        case Instruction::SetGlobalI:
        case Instruction::CallI:
        case Instruction::TailCallI: {
          auto [_, offset] = getImmediate(instructions, pc);
          pc += offset;
          break;
        }
        case Instruction::JumpI:
        case Instruction::JumpFalseI: {
          auto [immediate, offset] = getImmediate(instructions, pc);
          labelIndices.insert(pc + immediate);
          pc += offset;
          break;
        }
        case Instruction::GetParentI: {
          auto [_, offset] = getImmediate(instructions, pc + 1);
          pc += offset + 1;
          break;
        }
        case Instruction::BuiltinUnaryOp:
        case Instruction::BinaryOp:
        case Instruction::ConstMisc: {
          pc += 2;
          break;
        }
        case Instruction::ConstNum: {
          pc += sizeof(double) + 1;
          break;
        }
        case Instruction::Pop:
        case Instruction::Dup:
        case Instruction::Ret:
        case Instruction::Echo: {
          pc += 1;
          break;
        }
      }
    }
    pc = 0;
  }

  while (pc < instructions.size()) {
    Instruction inst = static_cast<Instruction>(instructions[pc]);
    if (labels) {
      auto iter = labelIndices.find(pc);
      if (iter != labelIndices.end())
        ostream << "l" << std::distance(labelIndices.begin(), iter) << ":"
                << std::endl;
      ostream << "  ";
    }
    switch (inst) {
      case Instruction::AddI:
      case Instruction::GetI:
      case Instruction::SetI:
      case Instruction::GetGlobalI:
      case Instruction::SetGlobalI:
      case Instruction::CallI:
      case Instruction::TailCallI: {
        auto [immediate, offset] = getImmediate(instructions, pc);
        ostream << getInstName(inst) << " " << immediate << std::endl;
        pc += offset;
        break;
      }
      case Instruction::JumpI:
      case Instruction::JumpFalseI: {
        auto [immediate, offset] = getImmediate(instructions, pc);
        ostream << getInstName(inst) << " ";
        if (labels)
          ostream << "l"
                  << std::distance(labelIndices.begin(),
                                   labelIndices.find(pc + immediate));
        else
          ostream << immediate;
        ostream << std::endl;
        pc += offset;
        break;
      }
      case Instruction::GetParentI: {
        unsigned char ancestor = instructions[pc + 1];
        auto [immediate, offset] = getImmediate(instructions, pc + 1);
        ostream << getInstName(inst) << " " << static_cast<int>(ancestor) << " "
                << immediate << std::endl;
        pc += offset + 1;
        break;
      }
      case Instruction::BuiltinUnaryOp: {
        ostream << getInstName(inst) << " "
                << static_cast<BuiltinUnary>(instructions[pc + 1]) << std::endl;
        pc += 2;
        break;
      }
      case Instruction::BinaryOp: {
        ostream << getInstName(inst) << " "
                << static_cast<BinOp>(instructions[pc + 1]) << std::endl;
        pc += 2;
        break;
      }
      case Instruction::ConstNum: {
        double v;
        memcpy(&v, instructions.data() + pc + 1, sizeof(double));
        ostream << getInstName(inst) << " " << v << std::endl;
        pc += sizeof(double) + 1;
        break;
      }
      case Instruction::ConstMisc: {
        ostream << getInstName(inst) << " ";
        switch (instructions[pc + 1]) {
          case 0:
            ostream << "false";
            break;
          case 1:
            ostream << "true";
            break;
          default:
            ostream << "undef";
            break;
        }
        ostream << std::endl;
        pc += 2;
        break;
      }
      case Instruction::Pop:
      case Instruction::Dup:
      case Instruction::Ret:
      case Instruction::Echo: {
        ostream << getInstName(inst) << std::endl;
        pc += 1;
        break;
      }
    }
  }
}
}  // namespace sscad
