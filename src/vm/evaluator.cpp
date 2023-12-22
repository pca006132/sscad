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
#include "evaluator.h"

#include <memory.h>

#include <iostream>

#include "ast.h"

using namespace std::string_literals;
using ValuePair = sscad::Evaluator::ValuePair;

namespace sscad {
constexpr char toHex(char n) {
  if (n < 10) return '0' + n;
  return 'A' + n - 10;
}

#define unimplemented() throw std::runtime_error("unimplemented");
#if defined(__GNUC__)
#define LIKELY(cond) __builtin_expect((cond), 1)
#define UNLIKELY(cond) __builtin_expect((cond), 0)
#else
#define LIKELY(cond) (cond)
#define UNLIKELY(cond) (cond)
#endif

void invalid() { throw std::runtime_error("invalid bytecode"); }

inline void drop(ValuePair v) {
  if (isAllocated(v.first)) unimplemented();
}

// return actual value and the pc increment for next function
std::pair<int, int> getImmediate(const FunctionEntry *entry, int currentPC) {
  if (UNLIKELY(currentPC + 1 >= entry->instructions.size())) invalid();
  if (LIKELY(entry->instructions[currentPC + 1] != 0xFF)) {
    const char *p = reinterpret_cast<const char *>(entry->instructions.data() +
                                                   currentPC + 1);
    return std::make_pair(*p, 2);
  }
  if (UNLIKELY(currentPC + 5 >= entry->instructions.size())) invalid();
  int p;
  memcpy(&p, entry->instructions.data() + currentPC + 2, sizeof(int));
  return std::make_pair(p, 6);
}

ValuePair handleUnary(ValuePair v, BuiltinUnary op) {
  switch (op) {
    case BuiltinUnary::NOT:
      // TODO: boolean cast
      if (v.first != ValueTag::BOOLEAN) unimplemented();
      return std::make_pair(ValueTag::BOOLEAN, SValue{.cond = !v.second.cond});
    case BuiltinUnary::NEG:
      if (v.first != ValueTag::NUMBER)
        return std::make_pair(ValueTag::UNDEF, SValue());
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = -v.second.number});
    default:
      unimplemented();
  }
}

ValuePair handleBinary(ValuePair lhs, ValuePair rhs, BinOp op) {
  bool equal = false;
  switch (op) {
    case BinOp::LT:
    case BinOp::LE:
      std::swap(lhs, rhs);
      equal = op == BinOp::LE;
      [[fallthrough]];
    case BinOp::GT:
    case BinOp::GE: {
      if (lhs.first != ValueTag::NUMBER || rhs.first != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return std::make_pair(ValueTag::UNDEF, SValue());
      }
      if (op == BinOp::GE) equal = true;
      bool result = lhs.second.number > rhs.second.number ||
                    (equal && lhs.second.number == rhs.second.number);
      return std::make_pair(ValueTag::BOOLEAN, SValue{.cond = result});
    }
    default:
      unimplemented();
  }
}

ValuePair Evaluator::eval(int id) {
  std::vector<ValueTag> tagStack;
  std::vector<SValue> valueStack;
  std::vector<int> rpStack({id});
  std::vector<int> spStack({0});
  std::vector<int> moduleSpStack({0});
  std::vector<int> pcStack({0});
  if (id >= functions.size()) invalid();
  const auto *fn = &functions[id];
  // note that we do not put the logical top stack element into the stack for
  // better performance.
  ValuePair top = std::make_pair(ValueTag::UNDEF, SValue());
  int pc = 0;

  auto bufferCheck = [&](int offset) {
    if (pc + offset >= fn->instructions.size()) invalid();
  };
  auto popSecond = [&] {
    if (tagStack.empty() || valueStack.empty()) invalid();
    ValuePair result = std::make_pair(tagStack.back(), valueStack.back());
    tagStack.pop_back();
    valueStack.pop_back();
    return result;
  };
  auto saveTop = [&] {
    tagStack.push_back(top.first);
    valueStack.push_back(top.second);
  };

  while (pc < fn->instructions.size()) {
    // std::cout << "pc: " << pc << std::endl;
    auto [immediate, offset] = (fn->instructions[pc] >= NO_IMMEDIATE_START)
                                   ? std::make_pair(0, 0)
                                   : getImmediate(fn, pc);
    Instruction inst = static_cast<Instruction>(fn->instructions[pc]);
    switch (inst) {
      case Instruction::AddI: {
        if (top.first == ValueTag::NUMBER)
          top.second.number += immediate;
        else
          unimplemented();
        pc += offset;
        break;
      }
      case Instruction::GetI:
      case Instruction::GetParentI: {
        saveTop();
        if (inst == Instruction::GetParentI && moduleSpStack.size() == 1)
          invalid();
        int index = (inst == Instruction::GetI
                         ? spStack.back()
                         : moduleSpStack[moduleSpStack.size() - 2]) +
                    immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        top = std::make_pair(tagStack[index], valueStack[index]);
        pc += offset;
        break;
      }
      case Instruction::SetI: {
        int index = spStack.back() + immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        tagStack[index] = top.first;
        valueStack[index] = top.second;
        top = popSecond();
        pc += offset;
        break;
      }
      case Instruction::GetGlobalI: {
        saveTop();
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        top = std::make_pair(globalTags[immediate], globalValues[immediate]);
        pc += offset;
        break;
      }
      case Instruction::SetGlobalI: {
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        globalTags[immediate] = top.first;
        globalValues[immediate] = top.second;
        top = popSecond();
        pc += offset;
        break;
      }
      case Instruction::JumpI:
      case Instruction::JumpTrueI: {
        int target = pc + immediate;
        if (target < 0 || target >= fn->instructions.size()) invalid();
        if (inst == Instruction::JumpTrueI) {
          // boolean cast
          if (top.first != ValueTag::BOOLEAN) unimplemented();
          if (!top.second.cond) target = pc + offset;
          top = popSecond();
        }
        pc = target;
        break;
      }
      case Instruction::CallI: {
        if (immediate >= functions.size()) invalid();
        const auto *fn = &functions[immediate];
        saveTop();
        pcStack.push_back(pc + offset);
        rpStack.push_back(immediate);
        spStack.push_back(valueStack.size() - fn->parameters);
        if (fn->isModule) moduleSpStack.push_back(spStack.back());
        pc = 0;
        break;
      }
      case Instruction::BuiltinUnaryOp: {
        bufferCheck(1);
        BuiltinUnary op = static_cast<BuiltinUnary>(fn->instructions[pc + 1]);
        top = handleUnary(top, op);
        pc += 2;
        break;
      }
      case Instruction::BinaryOp: {
        bufferCheck(1);
        BinOp op = static_cast<BinOp>(fn->instructions[pc + 1]);
        if (tagStack.empty() || valueStack.empty()) invalid();
        top = handleBinary(popSecond(), top, op);
        pc += 2;
        break;
      }
      case Instruction::ConstNum: {
        bufferCheck(1 + sizeof(double));
        double v;
        memcpy(&v, fn->instructions.data() + pc + 1, sizeof(double));
        saveTop();
        top = std::make_pair(ValueTag::NUMBER, SValue{.number = v});
        pc += sizeof(double) + 1;
        break;
      }
      case Instruction::ConstMisc: {
        bufferCheck(1);
        saveTop();
        switch (fn->instructions[pc + 1]) {
          case 0:
            top = std::make_pair(ValueTag::BOOLEAN, SValue{.cond = false});
            break;
          case 1:
            top = std::make_pair(ValueTag::BOOLEAN, SValue{.cond = true});
            break;
          default:
            top = std::make_pair(ValueTag::UNDEF, SValue());
        }
        pc += 2;
        break;
      }
      case Instruction::Pop: {
        drop(top);
        top = popSecond();
        pc += 1;
        break;
      }
      case Instruction::Dup: {
        saveTop();
        pc += 1;
        break;
      }
      case Instruction::Ret: {
        rpStack.pop_back();
        int sp = spStack.back();
        spStack.pop_back();
        for (int i = sp; i < valueStack.size(); i++) {
          drop(std::make_pair(tagStack[i], valueStack[i]));
        }
        tagStack.resize(sp + 1);
        valueStack.resize(sp + 1);
        if (pcStack.size() == 1) return top;
        fn = &functions[rpStack.back()];
        if (fn->isModule) moduleSpStack.pop_back();
        pc = pcStack.back();
        pcStack.pop_back();
        break;
      }
      case Instruction::Echo: {
        if (top.first != ValueTag::NUMBER) unimplemented();
        *ostream << top.second.number << std::endl;
        pc += 1;
        break;
      }
      default:
        throw std::runtime_error("unknown bytecode "s +
                                 toHex(fn->instructions.size()));
    }
  }
  throw std::runtime_error("evaluator stuck");
}
}  // namespace sscad
