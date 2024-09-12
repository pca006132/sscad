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

#include <cmath>
#include <cstring>
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
#define ALWAYS_INLINE __attribute__((always_inline))
#define COLD __attribute__((cold))
#else
#define LIKELY(cond) (cond)
#define UNLIKELY(cond) (cond)
#define ALWAYS_INLINE
#define COLD
#endif

COLD void invalid() { throw std::runtime_error("invalid bytecode"); }

constexpr ValuePair undef() {
  return std::make_pair(ValueTag::UNDEF, SValue());
}

constexpr ValuePair value(double number) {
  return std::make_pair(ValueTag::NUMBER, SValue{.number = number});
}

constexpr ValuePair value(size_t number) {
  return std::make_pair(ValueTag::NUMBER,
                        SValue{.number = static_cast<double>(number)});
}

constexpr ValuePair value(bool b) {
  return std::make_pair(ValueTag::BOOLEAN, SValue{.cond = b});
}

ALWAYS_INLINE ValuePair copy(ValuePair v) {
  if (isAllocated(v.first)) {
    switch (v.first) {
      case ValueTag::VECTOR:
        return std::make_pair(ValueTag::VECTOR,
                              SValue{.vec = new SVector{v.second.vec->values}});
      case ValueTag::RANGE:
        return std::make_pair(ValueTag::RANGE,
                              SValue{.range = new SRange(*v.second.range)});
      default:
        unimplemented();
    }
  }
  return v;
}

ALWAYS_INLINE void drop(ValuePair v) {
  if (isAllocated(v.first)) {
    switch (v.first) {
      case ValueTag::VECTOR:
        v.second.vec->values.reset();
        delete v.second.vec;
        break;
      case ValueTag::RANGE:
        delete v.second.range;
        break;
      default:
        unimplemented();
    }
  }
}

// return actual value and the pc increment for next function
ALWAYS_INLINE std::pair<int, int> getImmediate(const FunctionEntry *entry,
                                               int currentPC) {
  if (UNLIKELY(currentPC + 1 >= entry->instructions.size())) invalid();
  if (LIKELY(entry->instructions[currentPC + 1] != 0x80)) {
    const char *p = reinterpret_cast<const char *>(entry->instructions.data() +
                                                   currentPC + 1);
    return std::make_pair(static_cast<int>(*p), 2);
  }
  if (UNLIKELY(currentPC + 5 >= entry->instructions.size())) invalid();
  int p;
  memcpy(&p, entry->instructions.data() + currentPC + 2, sizeof(int));
  return std::make_pair(p, 6);
}

ALWAYS_INLINE ValuePair handleUnary(ValuePair v, BuiltinUnary op) {
  switch (op) {
    case BuiltinUnary::NOT:
      // TODO: boolean cast
      if (v.first != ValueTag::BOOLEAN) unimplemented();
      return value(!v.second.cond);
    case BuiltinUnary::NORM:
      unimplemented();
      break;
    case BuiltinUnary::LEN: {
      if (v.first != ValueTag::VECTOR) {
        drop(v);
        return undef();
      }
      auto s = v.second.vec->values->size();
      drop(v);
      return value(s);
    }
    case BuiltinUnary::RBEGIN:
    case BuiltinUnary::RSTEP:
    case BuiltinUnary::REND: {
      if (v.first != ValueTag::RANGE) {
        drop(v);
        return undef();
      }
      auto s = (op == BuiltinUnary::RBEGIN  ? v.second.range->begin
                : op == BuiltinUnary::RSTEP ? v.second.range->step
                                            : v.second.range->end);
      drop(v);
      return value(s);
    }
    default:
      break;
  }

  if (v.first != ValueTag::NUMBER) {
    drop(v);
    return undef();
  }
  switch (op) {
    case BuiltinUnary::NEG:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = -v.second.number});
    case BuiltinUnary::SIN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::sin(v.second.number)});
    case BuiltinUnary::COS:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::cos(v.second.number)});
    case BuiltinUnary::TAN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::tan(v.second.number)});
    case BuiltinUnary::ASIN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::asin(v.second.number)});
    case BuiltinUnary::ACOS:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::acos(v.second.number)});
    case BuiltinUnary::ATAN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::atan(v.second.number)});
    case BuiltinUnary::ABS:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::abs(v.second.number)});
    case BuiltinUnary::CEIL:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::ceil(v.second.number)});
    case BuiltinUnary::FLOOR:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::floor(v.second.number)});
    case BuiltinUnary::LN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::log(v.second.number)});
    case BuiltinUnary::LOG:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::log10(v.second.number)});
    case BuiltinUnary::ROUND:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::round(v.second.number)});
    case BuiltinUnary::SIGN:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = v.second.number == 0.0  ? 0.0
                                             : v.second.number > 0.0 ? 1.0
                                                                     : -1.0});
    case BuiltinUnary::SQRT:
      return std::make_pair(ValueTag::NUMBER,
                            SValue{.number = std::sqrt(v.second.number)});
    default:
      unimplemented();
  }
}

bool operator==(ValuePair lhs, ValuePair rhs) {
  if (lhs.first != rhs.first) return false;
  switch (lhs.first) {
    case ValueTag::STRING:
      return lhs.second.s == rhs.second.s;
    case ValueTag::VECTOR:
      if (lhs.second.vec->values->size() != rhs.second.vec->values->size())
        return false;
      for (size_t i = 0; i < lhs.second.vec->values->size(); i++) {
        if (lhs.second.vec->values->at(i) != rhs.second.vec->values->at(i))
          return false;
      }
      return true;
    case ValueTag::RANGE:
      return *lhs.second.range == *rhs.second.range;
    case ValueTag::NUMBER:
      return lhs.second.number == rhs.second.number;
    case ValueTag::BOOLEAN:
      return lhs.second.cond == rhs.second.cond;
    default:
      return true;
  }
}

ALWAYS_INLINE ValuePair handleBinary(ValuePair lhs, ValuePair rhs, BinOp op) {
  bool equal = false;
  switch (op) {
    case BinOp::ADD:
    case BinOp::SUB:
    case BinOp::MUL:
    case BinOp::DIV:
    case BinOp::MOD:
    case BinOp::EXP:
      if (lhs.first != ValueTag::NUMBER || rhs.first != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      switch (op) {
        case BinOp::ADD:
          return value(lhs.second.number + rhs.second.number);
        case BinOp::SUB:
          return value(lhs.second.number - rhs.second.number);
        case BinOp::MUL:
          return value(lhs.second.number * rhs.second.number);
        case BinOp::DIV:
          return value(lhs.second.number / rhs.second.number);
        case BinOp::MOD:
          return value(std::fmod(lhs.second.number, rhs.second.number));
        case BinOp::EXP:
          return value(std::pow(lhs.second.number, rhs.second.number));
        default:
          // impossible...
          unimplemented();
      }
    case BinOp::LT:
    case BinOp::LE:
      std::swap(lhs, rhs);
      equal = op == BinOp::LE;
      [[fallthrough]];
    case BinOp::GT:
    case BinOp::GE:
      if (lhs.first != ValueTag::NUMBER || rhs.first != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      if (op == BinOp::GE) equal = true;
      return value(lhs.second.number > rhs.second.number ||
                   (equal && lhs.second.number == rhs.second.number));

    case BinOp::EQ:
    case BinOp::NEQ: {
      bool result = lhs == rhs;
      drop(lhs);
      drop(rhs);
      return value(result);
    }
    case BinOp::AND:
    case BinOp::OR: {
      if (lhs.first != ValueTag::BOOLEAN || rhs.first != ValueTag::BOOLEAN) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      return value(op == BinOp::AND ? (lhs.second.cond && rhs.second.cond)
                                    : (lhs.second.cond || rhs.second.cond));
    }
    case BinOp::APPEND:
      if (lhs.first != ValueTag::VECTOR) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      if (lhs.second.vec->values.use_count() != 1) {
        auto lhsClone = new SVector{
            std::make_shared<std::vector<ValuePair>>(*lhs.second.vec->values)};
        drop(lhs);
        lhs = std::make_pair(ValueTag::VECTOR, SValue{.vec = lhsClone});
      }
      lhs.second.vec->values->push_back(rhs);
      return lhs;
    case BinOp::CONCAT:
      if (lhs.first != ValueTag::VECTOR || rhs.first != ValueTag::VECTOR) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      if (lhs.second.vec->values.use_count() != 1) {
        auto lhsClone = new SVector{
            std::make_shared<std::vector<ValuePair>>(*lhs.second.vec->values)};
        drop(lhs);
        lhs = std::make_pair(ValueTag::VECTOR, SValue{.vec = lhsClone});
      }
      lhs.second.vec->values->insert(lhs.second.vec->values->end(),
                                     rhs.second.vec->values->begin(),
                                     rhs.second.vec->values->end());
      return lhs;
    case BinOp::INDEX: {
      if (lhs.first != ValueTag::VECTOR || rhs.first != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return undef();
      }
      auto index = static_cast<size_t>(rhs.second.number);
      if (index < 0 || index >= lhs.second.vec->values->size()) {
        drop(lhs);
        return undef();
      }
      auto value = copy(lhs.second.vec->values->at(index));
      drop(lhs);
      return value;
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
  std::vector<int> pcStack({0});
  if (id >= functions.size()) invalid();
  const auto *fn = &functions[id];
  // note that we do not put the logical top stack element into the stack for
  // better performance.
  ValuePair top = std::make_pair(ValueTag::UNDEF, SValue());
  bool notop = true;
  int pc = 0;

  auto bufferCheck = [&](int offset) {
    if (UNLIKELY(pc + offset >= fn->instructions.size())) invalid();
  };
  auto popSecond = [&] {
    if (UNLIKELY(tagStack.empty() || valueStack.empty())) invalid();
    ValuePair result = std::make_pair(tagStack.back(), valueStack.back());
    tagStack.pop_back();
    valueStack.pop_back();
    return result;
  };
  auto saveTop = [&] {
    if (UNLIKELY(notop)) {
      notop = false;
      return;
    }
    tagStack.push_back(top.first);
    valueStack.push_back(top.second);
  };

  long counter = 0;
  while (true) {
    counter++;
    Instruction inst = static_cast<Instruction>(fn->instructions[pc]);
    switch (inst) {
      case Instruction::GetI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        saveTop();
        int index = spStack.back() + immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        top = copy(std::make_pair(tagStack[index], valueStack[index]));
        pc += offset;
        break;
      }
      case Instruction::AddI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (LIKELY(top.first == ValueTag::NUMBER))
          top.second.number += immediate;
        else
          unimplemented();
        pc += offset;
        break;
      }
      case Instruction::SetI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        int index = spStack.back() + immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        tagStack[index] = top.first;
        valueStack[index] = top.second;
        top = popSecond();
        pc += offset;
        break;
      }
      case Instruction::JumpI:
      case Instruction::JumpFalseI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        int target = pc + immediate;
        if (target < 0 || target >= fn->instructions.size()) invalid();
        if (inst == Instruction::JumpFalseI) {
          // boolean cast
          if (top.first != ValueTag::BOOLEAN) unimplemented();
          if (top.second.cond) target = pc + offset;
          top = popSecond();
        }
        pc = target;
        break;
      }
      case Instruction::Iter: {
        auto [immediate, offset] = getImmediate(fn, pc);
        int target = pc + immediate;
        if (target < 0 || target >= fn->instructions.size()) invalid();
        if (tagStack.empty() || valueStack.empty()) invalid();
        if (top.first != ValueTag::NUMBER) invalid();
        top.second.number += 1;
        if (tagStack.back() == ValueTag::VECTOR) {
          if (valueStack.back().vec->values->size() <= top.second.number) {
            drop(popSecond());
            top = popSecond();
          } else {
            auto elem =
                copy(valueStack.back().vec->values->at(top.second.number));
            saveTop();
            top = elem;
            target = pc + offset;
          }
        } else if (tagStack.back() == ValueTag::RANGE) {
          auto r = *valueStack.back().range;
          auto newValue = top.second.number * r.step + r.begin;
          if (newValue > r.end) {
            drop(popSecond());
            top = popSecond();
          } else {
            saveTop();
            top = value(newValue);
            target = pc + offset;
          }
        } else {
          invalid();
        }
        pc = target;
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
        top = copy(top);
        pc += 1;
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
      case Instruction::GetGlobalI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        saveTop();
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        top = copy(
            std::make_pair(globalTags[immediate], globalValues[immediate]));
        pc += offset;
        break;
      }
      case Instruction::SetGlobalI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        globalTags[immediate] = top.first;
        globalValues[immediate] = top.second;
        top = popSecond();
        pc += offset;
        break;
      }
      case Instruction::CallI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (immediate >= functions.size()) invalid();
        fn = &functions[immediate];
        saveTop();
        pcStack.push_back(pc + offset);
        rpStack.push_back(immediate);
        spStack.push_back(valueStack.size() - fn->parameters);
        pc = 0;
        notop = true;
        break;
      }
      case Instruction::TailCallI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (immediate >= functions.size()) invalid();
        fn = &functions[immediate];
        saveTop();

        int sp = spStack.back();
        int params = fn->parameters;
        int stackEnd = valueStack.size() - params;
        for (int i = sp; i < stackEnd; i++) {
          drop(std::make_pair(tagStack[i], valueStack[i]));
        }
        for (int i = 0; i < params; i++) {
          tagStack[sp + i] = tagStack[stackEnd + i];
          valueStack[sp + i] = valueStack[stackEnd + i];
        }
        tagStack.resize(sp + params);
        valueStack.resize(sp + params);
        rpStack.back() = immediate;
        pc = 0;
        notop = true;
        break;
      }
      case Instruction::Ret: {
        // this is undefined behavior
        if (notop) invalid();
        rpStack.pop_back();
        int sp = spStack.back();
        spStack.pop_back();
        for (int i = sp; i < valueStack.size(); i++) {
          drop(std::make_pair(tagStack[i], valueStack[i]));
        }
        tagStack.resize(sp + 1);
        valueStack.resize(sp + 1);
        if (pcStack.size() == 1) {
          // *ostream << "instructions executed: " << counter << std::endl;
          return top;
        }
        fn = &functions[rpStack.back()];
        pc = pcStack.back();
        pcStack.pop_back();
        break;
      }
      case Instruction::MakeRange: {
        if (tagStack.size() <= 1 || valueStack.size() <= 1) invalid();
        auto step = popSecond();
        auto start = popSecond();
        auto end = top;
        if (start.first != ValueTag::NUMBER || step.first != ValueTag::NUMBER ||
            end.first != ValueTag::NUMBER) {
          drop(start);
          drop(step);
          drop(end);
          top = undef();
        } else {
          top = std::make_pair(ValueTag::RANGE,
                               SValue{.range = new SRange{start.second.number,
                                                          step.second.number,
                                                          end.second.number}});
        }
        pc += 1;
      }
      case Instruction::MakeList: {
        saveTop();
        top = std::make_pair(
            ValueTag::VECTOR,
            SValue{.vec = new SVector{
                       std::make_shared<std::vector<ValuePair>>()}});
        pc += 1;
      }
      case Instruction::Echo: {
        if (top.first != ValueTag::NUMBER) unimplemented();
        *ostream << top.second.number << std::endl;
        pc += 1;
        break;
      }
      default:
        throw std::runtime_error("unknown bytecode "s +
                                 toHex(fn->instructions[pc]));
    }
  }
  if (!flag.load(std::memory_order_relaxed)) {
    *ostream << "instructions executed: " << counter << std::endl;
    return std::make_pair(ValueTag::UNDEF, SValue());
  }
  throw std::runtime_error("evaluator stuck");
}
}  // namespace sscad
