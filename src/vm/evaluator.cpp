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
#include "instructions.h"

using namespace std::string_literals;

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

ALWAYS_INLINE ValuePair copy(ValuePair v) {
  if (isAllocated(v.tag)) {
    switch (v.tag) {
      case ValueTag::VECTOR:
        return ValuePair(ValueTag::VECTOR,
                         SValue{.vec = new SVector{v.value.vec->values}});
      case ValueTag::RANGE:
        return ValuePair(ValueTag::RANGE,
                         SValue{.range = new SRange(*v.value.range)});
      default:
        unimplemented();
    }
  }
  return v;
}

ALWAYS_INLINE void drop(ValuePair v) {
  if (isAllocated(v.tag)) {
    switch (v.tag) {
      case ValueTag::VECTOR:
        v.value.vec->values.reset();
        delete v.value.vec;
        break;
      case ValueTag::RANGE:
        delete v.value.range;
        break;
      default:
        unimplemented();
    }
  }
}

struct ImmediatePair {
  int immediate;
  int offset;
};

// return actual value and the pc increment for next function
ALWAYS_INLINE ImmediatePair getImmediate(const FunctionEntry *entry,
                                         int currentPC) {
  if (UNLIKELY(currentPC + 1 >= entry->instructions.size())) invalid();
  if (LIKELY(entry->instructions[currentPC + 1] != 0x80)) {
    const char *p = reinterpret_cast<const char *>(entry->instructions.data() +
                                                   currentPC + 1);
    return ImmediatePair{static_cast<int>(*p), 2};
  }
  if (UNLIKELY(currentPC + 5 >= entry->instructions.size())) invalid();
  int p;
  memcpy(&p, entry->instructions.data() + currentPC + 2, sizeof(int));
  return ImmediatePair{p, 6};
}

ALWAYS_INLINE ValuePair handleUnary(ValuePair v, BuiltinUnary op) {
  switch (op) {
    case BuiltinUnary::NOT:
      // TODO: boolean cast
      if (v.tag != ValueTag::BOOLEAN) unimplemented();
      return ValuePair(!v.value.cond);
    case BuiltinUnary::NORM:
      unimplemented();
      break;
    case BuiltinUnary::LEN: {
      if (v.tag != ValueTag::VECTOR) {
        drop(v);
        return ValuePair::undef();
      }
      auto s = v.value.vec->values->size();
      drop(v);
      return ValuePair(s);
    }
    case BuiltinUnary::RBEGIN:
    case BuiltinUnary::RSTEP:
    case BuiltinUnary::REND: {
      if (v.tag != ValueTag::RANGE) {
        drop(v);
        return ValuePair::undef();
      }
      auto s = (op == BuiltinUnary::RBEGIN  ? v.value.range->begin
                : op == BuiltinUnary::RSTEP ? v.value.range->step
                                            : v.value.range->end);
      drop(v);
      return ValuePair(s);
    }
    default:
      break;
  }

  if (v.tag != ValueTag::NUMBER) {
    drop(v);
    return ValuePair::undef();
  }
  switch (op) {
    case BuiltinUnary::NEG:
      return ValuePair(-v.value.number);
    case BuiltinUnary::SIN:
      return ValuePair(std::sin(v.value.number));
    case BuiltinUnary::COS:
      return ValuePair(std::cos(v.value.number));
    case BuiltinUnary::TAN:
      return ValuePair(std::tan(v.value.number));
    case BuiltinUnary::ASIN:
      return ValuePair(std::asin(v.value.number));
    case BuiltinUnary::ACOS:
      return ValuePair(std::acos(v.value.number));
    case BuiltinUnary::ATAN:
      return ValuePair(std::atan(v.value.number));
    case BuiltinUnary::ABS:
      return ValuePair(std::abs(v.value.number));
    case BuiltinUnary::CEIL:
      return ValuePair(std::ceil(v.value.number));
    case BuiltinUnary::FLOOR:
      return ValuePair(std::floor(v.value.number));
    case BuiltinUnary::LN:
      return ValuePair(std::log(v.value.number));
    case BuiltinUnary::LOG:
      return ValuePair(std::log10(v.value.number));
    case BuiltinUnary::ROUND:
      return ValuePair(std::round(v.value.number));
    case BuiltinUnary::SIGN:
      return ValuePair(v.value.number == 0.0  ? 0.0
                       : v.value.number > 0.0 ? 1.0
                                              : -1.0);
    case BuiltinUnary::SQRT:
      return ValuePair(std::sqrt(v.value.number));
    default:
      unimplemented();
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
      if (lhs.tag != ValueTag::NUMBER || rhs.tag != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      switch (op) {
        case BinOp::ADD:
          return ValuePair(lhs.value.number + rhs.value.number);
        case BinOp::SUB:
          return ValuePair(lhs.value.number - rhs.value.number);
        case BinOp::MUL:
          return ValuePair(lhs.value.number * rhs.value.number);
        case BinOp::DIV:
          return ValuePair(lhs.value.number / rhs.value.number);
        case BinOp::MOD:
          return ValuePair(std::fmod(lhs.value.number, rhs.value.number));
        case BinOp::EXP:
          return ValuePair(std::pow(lhs.value.number, rhs.value.number));
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
      if (lhs.tag != ValueTag::NUMBER || rhs.tag != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      if (op == BinOp::GE) equal = true;
      return ValuePair(lhs.value.number > rhs.value.number ||
                       (equal && lhs.value.number == rhs.value.number));

    case BinOp::EQ:
    case BinOp::NEQ: {
      bool result = lhs == rhs;
      drop(lhs);
      drop(rhs);
      return ValuePair(result);
    }
    case BinOp::AND:
    case BinOp::OR: {
      if (lhs.tag != ValueTag::BOOLEAN || rhs.tag != ValueTag::BOOLEAN) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      return ValuePair(op == BinOp::AND ? (lhs.value.cond && rhs.value.cond)
                                        : (lhs.value.cond || rhs.value.cond));
    }
    case BinOp::APPEND:
      if (lhs.tag != ValueTag::VECTOR) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      if (lhs.value.vec->values.use_count() != 1) {
        auto lhsClone = new SVector{
            std::make_shared<std::vector<ValuePair>>(*lhs.value.vec->values)};
        drop(lhs);
        lhs = ValuePair(ValueTag::VECTOR, SValue{.vec = lhsClone});
      }
      lhs.value.vec->values->push_back(rhs);
      return lhs;
    case BinOp::CONCAT:
      if (lhs.tag != ValueTag::VECTOR || rhs.tag != ValueTag::VECTOR) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      if (lhs.value.vec->values.use_count() != 1) {
        auto lhsClone = new SVector{
            std::make_shared<std::vector<ValuePair>>(*lhs.value.vec->values)};
        drop(lhs);
        lhs = ValuePair(ValueTag::VECTOR, SValue{.vec = lhsClone});
      }
      lhs.value.vec->values->insert(lhs.value.vec->values->end(),
                                    rhs.value.vec->values->begin(),
                                    rhs.value.vec->values->end());
      return lhs;
    case BinOp::INDEX: {
      if (lhs.tag != ValueTag::VECTOR || rhs.tag != ValueTag::NUMBER) {
        drop(lhs);
        drop(rhs);
        return ValuePair::undef();
      }
      auto index = static_cast<size_t>(rhs.value.number);
      if (index < 0 || index >= lhs.value.vec->values->size()) {
        drop(lhs);
        return ValuePair::undef();
      }
      auto value = copy(lhs.value.vec->values->at(index));
      drop(lhs);
      return value;
    }
    default:
      unimplemented();
  }
}

inline ValuePair popvalue(std::vector<ValueTag> &tagStack,
                                        std::vector<SValue> &valueStack) {
  if (UNLIKELY(tagStack.empty() || valueStack.empty())) invalid();
  ValuePair result = ValuePair(tagStack.back(), valueStack.back());
  tagStack.pop_back();
  valueStack.pop_back();
  return result;
}

inline void saveTop(bool &notop, const ValuePair &top,
                                  std::vector<ValueTag> &tagStack,
                                  std::vector<SValue> &valueStack) {
  if (UNLIKELY(notop)) {
    notop = false;
    return;
  }
  tagStack.push_back(top.tag);
  valueStack.push_back(top.value);
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
  ValuePair top = ValuePair::undef();
  bool notop = true;
  int pc = 0;

  auto bufferCheck = [&](int offset) {
    if (UNLIKELY(pc + offset >= fn->instructions.size())) invalid();
  };

  long counter = 0;
  while (true) {
    Instruction inst = static_cast<Instruction>(fn->instructions[pc]);
    counter++;
    switch (inst) {
      case Instruction::GetI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        saveTop(notop, top, tagStack, valueStack);
        int index = spStack.back() + immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        top = copy(ValuePair(tagStack[index], valueStack[index]));
        pc += offset;
        break;
      }
      case Instruction::AddI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (LIKELY(top.tag == ValueTag::NUMBER))
          top.value.number += immediate;
        else
          unimplemented();
        pc += offset;
        break;
      }
      case Instruction::SetI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        int index = spStack.back() + immediate;
        if (index < 0 || index >= tagStack.size()) invalid();
        tagStack[index] = top.tag;
        valueStack[index] = top.value;
        top = popvalue(tagStack, valueStack);
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
          if (top.tag != ValueTag::BOOLEAN) unimplemented();
          if (top.value.cond) target = pc + offset;
          top = popvalue(tagStack, valueStack);
        }
        pc = target;
        break;
      }
      case Instruction::Iter: {
        auto [immediate, offset] = getImmediate(fn, pc);
        int target = pc + immediate;
        if (target < 0 || target >= fn->instructions.size()) invalid();
        if (tagStack.empty() || valueStack.empty()) invalid();
        if (top.tag != ValueTag::NUMBER) invalid();
        top.value.number += 1;
        if (tagStack.back() == ValueTag::VECTOR) {
          if (valueStack.back().vec->values->size() <= top.value.number) {
            drop(popvalue(tagStack, valueStack));
            top = popvalue(tagStack, valueStack);
          } else {
            auto elem =
                copy(valueStack.back().vec->values->at(top.value.number));
            saveTop(notop, top, tagStack, valueStack);
            top = elem;
            target = pc + offset;
          }
        } else if (tagStack.back() == ValueTag::RANGE) {
          auto r = *valueStack.back().range;
          auto newValue = top.value.number * r.step + r.begin;
          if (newValue > r.end) {
            drop(popvalue(tagStack, valueStack));
            top = popvalue(tagStack, valueStack);
          } else {
            saveTop(notop, top, tagStack, valueStack);
            top = ValuePair(newValue);
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
        top = popvalue(tagStack, valueStack);
        pc += 1;
        break;
      }
      case Instruction::Dup: {
        saveTop(notop, top, tagStack, valueStack);
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
        top = handleBinary(popvalue(tagStack, valueStack), top, op);
        pc += 2;
        break;
      }
      case Instruction::ConstNum: {
        bufferCheck(1 + sizeof(double));
        double v;
        memcpy(&v, fn->instructions.data() + pc + 1, sizeof(double));
        saveTop(notop, top, tagStack, valueStack);
        top = ValuePair(v);
        pc += sizeof(double) + 1;
        break;
      }
      case Instruction::ConstMisc: {
        bufferCheck(1);
        saveTop(notop, top, tagStack, valueStack);
        switch (fn->instructions[pc + 1]) {
          case 0:
            top = ValuePair(false);
            break;
          case 1:
            top = ValuePair(true);
            break;
          default:
            top = ValuePair::undef();
        }
        pc += 2;
        break;
      }
      case Instruction::GetGlobalI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        saveTop(notop, top, tagStack, valueStack);
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        top = copy(ValuePair(globalTags[immediate], globalValues[immediate]));
        pc += offset;
        break;
      }
      case Instruction::SetGlobalI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (immediate < 0 || immediate >= globalTags.size()) invalid();
        globalTags[immediate] = top.tag;
        globalValues[immediate] = top.value;
        top = popvalue(tagStack, valueStack);
        pc += offset;
        break;
      }
      case Instruction::CallI: {
        auto [immediate, offset] = getImmediate(fn, pc);
        if (immediate >= functions.size()) invalid();
        fn = &functions[immediate];
        saveTop(notop, top, tagStack, valueStack);
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
        saveTop(notop, top, tagStack, valueStack);

        int sp = spStack.back();
        int params = fn->parameters;
        int stackEnd = valueStack.size() - params;
        for (int i = sp; i < stackEnd; i++) {
          drop(ValuePair(tagStack[i], valueStack[i]));
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
          drop(ValuePair(tagStack[i], valueStack[i]));
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
        auto step = popvalue(tagStack, valueStack);
        auto start = popvalue(tagStack, valueStack);
        auto end = top;
        if (start.tag != ValueTag::NUMBER || step.tag != ValueTag::NUMBER ||
            end.tag != ValueTag::NUMBER) {
          drop(start);
          drop(step);
          drop(end);
          top = ValuePair::undef();
        } else {
          top = ValuePair(
              ValueTag::RANGE,
              SValue{.range = new SRange{start.value.number, step.value.number,
                                         end.value.number}});
        }
        pc += 1;
      }
      case Instruction::MakeList: {
        saveTop(notop, top, tagStack, valueStack);
        top =
            ValuePair(ValueTag::VECTOR,
                      SValue{.vec = new SVector{
                                 std::make_shared<std::vector<ValuePair>>()}});
        pc += 1;
      }
      case Instruction::Echo: {
        if (top.tag != ValueTag::NUMBER) unimplemented();
        *ostream << top.value.number << std::endl;
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
    return ValuePair(ValueTag::UNDEF, SValue());
  }
  throw std::runtime_error("evaluator stuck");
}
}  // namespace sscad
