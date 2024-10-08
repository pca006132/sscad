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
#include <cmath>
#include <optional>
#include <unordered_map>
#include <vector>

#include "ast_visitor.h"
#include "frontend.h"

namespace sscad {
// TODO: handle undef
// we should also remove unused variables
class ConstEvaluator : public ExprMap {
 public:
  using ExprMap::map;
  using ExprMap::visit;
  virtual std::shared_ptr<ExprNode> map(UnaryOpNode& node) override {
    const auto operand = map(node.operand);
    auto number = dynamic_cast<NumberNode*>(operand.get());
    if (number != nullptr) {
      if (node.op == UnaryOp::NEG)
        return std::make_shared<NumberNode>(-number->value, node.loc);
      if (node.op == UnaryOp::NOT)
        return std::make_shared<NumberNode>(number->value != 0 ? 0 : 1,
                                            node.loc);
    }
    return std::make_shared<UnaryOpNode>(operand, node.op, node.loc);
  }

  virtual std::shared_ptr<ExprNode> map(BinaryOpNode& node) override {
    const auto lhs = map(node.lhs);
    const auto rhs = map(node.rhs);
    auto lhsNum = dynamic_cast<NumberNode*>(lhs.get());
    auto rhsNum = dynamic_cast<NumberNode*>(rhs.get());
    if (lhsNum != nullptr && rhsNum != nullptr) {
      const double lhs = lhsNum->value;
      const double rhs = rhsNum->value;
      double result = 0;
      switch (node.op) {
        case BinOp::ADD:
          result = lhs + rhs;
          break;
        case BinOp::SUB:
          result = lhs - rhs;
          break;
        case BinOp::MUL:
          result = lhs * rhs;
          break;
        case BinOp::DIV:
          if (fabs(rhs) == 0.0)
            result = NAN;
          else
            result = lhs / rhs;
          break;
        case BinOp::MOD:
          if (fabs(rhs) == 0.0)
            result = NAN;
          else
            result = std::fmod(lhs, rhs);
          break;
        case BinOp::EXP:
          result = std::pow(lhs, rhs);
          break;
        case BinOp::LT:
          result = lhs < rhs ? 1 : 0;
          break;
        case BinOp::LE:
          result = lhs <= rhs ? 1 : 0;
          break;
        case BinOp::GT:
          result = lhs > rhs ? 1 : 0;
          break;
        case BinOp::GE:
          result = lhs >= rhs ? 1 : 0;
          break;
        case BinOp::EQ:
          result = lhs == rhs ? 1 : 0;
          break;
        case BinOp::NEQ:
          result = lhs == rhs ? 0 : 1;
          break;
        case BinOp::AND:
          result = (lhs == 0 || rhs == 0) ? 0 : 1;
          break;
        case BinOp::OR:
          result = (lhs == 0 && rhs == 0) ? 0 : 1;
          break;
        default:
          throw std::runtime_error("invalid AST");
      }
      return std::make_shared<NumberNode>(result, node.loc);
    }
    return std::make_shared<BinaryOpNode>(lhs, rhs, node.op, node.loc);
  }

  virtual std::shared_ptr<ExprNode> map(IfExprNode& node) override {
    const auto cond = map(node.cond);
    auto number = dynamic_cast<NumberNode*>(cond.get());
    if (number != nullptr) {
      if (fabs(number->value) == 0.0) return map(node.ifelse);
      return map(node.ifthen);
    }
    return std::make_shared<IfExprNode>(cond, map(node.ifthen),
                                        map(node.ifelse), node.loc);
  }

  // TODO: store the list of available module, function and variable names, do
  // static checking.
  virtual void visit(ModuleBody& body) override {
    fixAssignments(body.assignments);
    for (auto& child : body.children) visit(child);
    variableLookup.pop_back();
  }

  virtual void visit(TranslationUnit& unit) override {
    fixAssignments(unit.assignments);
    for (auto& module : unit.modules) {
      visit(module);
    }
    for (auto& fun : unit.functions) {
      visit(fun);
    }
    for (auto& call : unit.moduleCalls) {
      visit(call);
    }
    variableLookup.pop_back();
  }

  void fixAssignments(std::vector<AssignNode>& assignments) {
    // remove duplicates and move assignments to the first occurrence
    std::unordered_map<std::string, size_t> localIndices;
    for (size_t i = 0; i < assignments.size(); ++i) {
      auto& assign = assignments[i];
      auto iter = localIndices.find(assign.ident);
      if (iter != localIndices.end()) {
        // TODO: custom warning type with two locations
        warnings.push_back(
            std::make_pair(assign.loc, "duplicated variable declaration"));
        assignments[iter->second] = assign;
        assignments.erase(assignments.begin() + i);
        i -= 1;
      } else {
        localIndices.insert(std::make_pair(assign.ident, i));
      }
    }
    variableLookup.push_back({});
    for (auto& assign : assignments) {
      visit(assign);
      // note that we only cache constant values, to avoid inlining making
      // the code too long
      variableLookup.back().insert(
          std::make_pair(assign.ident, assign.expr->isConstValue()
                                           ? std::make_optional(assign.expr)
                                           : std::nullopt));
    }
  }

 private:
  std::vector<std::unordered_map<std::string, std::optional<Expr>>>
      variableLookup;
  std::vector<std::pair<Location, std::string>> warnings;
};
}  // namespace sscad
