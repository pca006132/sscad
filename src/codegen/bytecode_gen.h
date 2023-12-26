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
#include <cassert>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "vm/instructions.h"

namespace sscad {
const std::unordered_map<std::string, BuiltinUnary> builtins = {
    {"sin", BuiltinUnary::SIN},     {"cos", BuiltinUnary::COS},
    {"tan", BuiltinUnary::TAN},     {"asin", BuiltinUnary::ASIN},
    {"acos", BuiltinUnary::ACOS},   {"atan", BuiltinUnary::ATAN},
    {"abs", BuiltinUnary::ABS},     {"ceil", BuiltinUnary::CEIL},
    {"floor", BuiltinUnary::FLOOR}, {"ln", BuiltinUnary::LN},
    {"log", BuiltinUnary::LOG},     {"norm", BuiltinUnary::NORM},
    {"round", BuiltinUnary::ROUND}, {"sign", BuiltinUnary::SIGN},
    {"sqrt", BuiltinUnary::SQRT},
};

// simple direct translation...
class BytecodeGen : public AstVisitor {
 public:
  virtual void visit(NumberNode& node) override {
    addDouble(tail->instructions, node.value);
  }

  virtual void visit(StringNode& node) override {
    // should just be some global value thing
    throw std::runtime_error("not implemented");
  }

  virtual void visit(UndefNode& node) override {
    addInst(tail->instructions, Instruction::ConstMisc, 2);
  }

  virtual void visit(IdentNode& node) override {
    // this is basically just variable node
    if (node.isConfigVar()) {
      const auto pair =
          std::make_pair(std::numeric_limits<FileHandle>::max(), node.name);
      auto iter = globalMap.insert(std::make_pair(pair, globalMap.size()));
      addInst(tail->instructions, Instruction::GetGlobalI, iter.first->second);
      return;
    }
    // check local scope
    {
      const auto iter = variableLookup.back().find(node.name);
      if (iter != variableLookup.back().end()) {
        addInst(tail->instructions, Instruction::GetI, iter->second);
        return;
      }
    }
    // check parent scopes
    for (int i = 1; i < variableLookup.size() - 1; i++) {
      const auto iter =
          variableLookup[variableLookup.size() - i - 1].find(node.name);
      if (iter != variableLookup.back().end()) {
        addInst(tail->instructions, Instruction::GetParentI);
        assert(i < 255);
        tail->instructions.push_back(static_cast<unsigned char>(i));
        addImm(tail->instructions, iter->second);
        return;
      }
    }
    // check file scope
    {
      const auto iter = globalMap.find(std::make_pair(currentFile, node.name));
      if (iter != globalMap.end()) {
        addInst(tail->instructions, Instruction::GetGlobalI, iter->second);
        return;
      }
    }
    warnings.emplace_back(node.loc, "undefined variable");
    // undef
    addInst(tail->instructions, Instruction::ConstMisc, 2);
  }

  virtual void visit(UnaryOpNode& node) override {
    node.operand->visit(*this);
    if (node.op == UnaryOp::NOT)
      addUnaryOp(tail->instructions, BuiltinUnary::NOT);
    else  // if (node.op == UnaryOp::NEG)
      addUnaryOp(tail->instructions, BuiltinUnary::NEG);
  }

  virtual void visit(BinaryOpNode& node) override {
    node.lhs->visit(*this);
    node.rhs->visit(*this);
    addBinOp(tail->instructions, node.op);
  }

  virtual void visit(CallNode& node) override {
    // don't care about multiple modules and lambda for now
    // also don't care about parameter reordering and such, will fix later...
    auto ident = dynamic_cast<IdentNode*>(node.fun.get());
    if (ident == nullptr)
      throw std::runtime_error("lambda not supported for now");
    for (auto& arg : node.args) {
      arg.expr->visit(*this);
    }
    auto iter = functionMap.find(std::make_pair(currentFile, ident->name));
    if (iter != functionMap.end()) {
      addInst(tail->instructions, Instruction::CallI, iter->second);
      return;
    }
    auto iter2 = builtins.find(ident->name);
    if (iter2 != builtins.end()) {
      addUnaryOp(tail->instructions, iter2->second);
      return;
    }
    throw std::runtime_error("unknown function call");
  }

  virtual void visit(IfExprNode& node) override {
    node.cond->visit(*this);
    int currentid = currentbb;

    bb.emplace_back();
    int falseid = bb.size() - 1;
    currentbb = falseid;
    tail = &bb[falseid];
    node.ifelse->visit(*this);

    bb.emplace_back();
    int trueid = bb.size() - 1;
    currentbb = trueid;
    tail = &bb[trueid];
    node.ifthen->visit(*this);

    bb.emplace_back();
    int tailid = bb.size() - 1;
    currentbb = tailid;
    tail = &bb[tailid];

    bb[trueid].next = tailid;
    bb[falseid].next = tailid;
    bb[currentid].jumpFalse = falseid;
    bb[currentid].next = trueid;
  }

 private:
  struct BasicBlock {
    std::vector<unsigned char> instructions;
    std::optional<int> jumpFalse;
    int next;
  };
  std::vector<std::unordered_map<std::string, int>> variableLookup;
  std::unordered_map<std::pair<FileHandle, std::string>, int> functionMap;
  std::unordered_map<std::pair<FileHandle, std::string>, int> globalMap;
  std::vector<std::pair<Location, std::string>> warnings;
  std::vector<BasicBlock> bb;
  BasicBlock* tail;
  unsigned int currentbb;
  FileHandle currentFile;
};
}  // namespace sscad
