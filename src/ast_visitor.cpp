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
#include "ast.h"
#include "frontend.h"

namespace sscad {
void AstVisitor::visit(AssignNode &node) {
  if (node.expr != nullptr) node.expr->visit(*this);
}

void AstVisitor::visit(ModuleBody &node) {
  for (auto &assign : node.assignments) assign.visit(*this);
  for (auto &child : node.children) child->visit(*this);
}

void AstVisitor::visit(SingleModuleCall &node) {
  for (auto &arg : node.args) arg.visit(*this);
  node.body.visit(*this);
}

void AstVisitor::visit(IfModule &node) {
  for (auto &arg : node.args) arg.visit(*this);
  node.ifthen.visit(*this);
  node.ifelse.visit(*this);
}

void AstVisitor::visit(ModuleModifier &node) { node.module->visit(*this); }

void AstVisitor::visit(ModuleDecl &node) {
  for (auto &arg : node.args) arg.visit(*this);
  node.body.visit(*this);
}

void AstVisitor::visit(FunctionDecl &node) {
  for (auto &arg : node.args) arg.visit(*this);
  node.body->visit(*this);
}

void AstVisitor::visit(UnaryOpNode &node) { node.operand->visit(*this); }

void AstVisitor::visit(BinaryOpNode &node) {
  node.lhs->visit(*this);
  node.rhs->visit(*this);
}

void AstVisitor::visit(CallNode &node) {
  node.fun->visit(*this);
  for (auto &arg : node.args) arg.visit(*this);
}

void AstVisitor::visit(IfExprNode &node) {
  node.cond->visit(*this);
  node.ifthen->visit(*this);
  node.ifelse->visit(*this);
}

void AstVisitor::visit(TranslationUnit &unit) {
  for (auto &module : unit.modules) {
    visit(module);
  }
  for (auto &fun : unit.functions) {
    visit(fun);
  }
  for (auto &assign : unit.assignments) {
    visit(assign);
  }
  for (auto &call : unit.moduleCalls) {
    call->visit(*this);
  }
}

void ExprMap::visit(AssignNode &node) {
  if (node.expr != nullptr) node.expr = node.expr->map(*this);
}
Expr ExprMap::map(NumberNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(StringNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(UndefNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(IdentNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(UnaryOpNode &node) {
  return std::make_shared<UnaryOpNode>(node.operand->map(*this), node.op,
                                       node.loc);
}
Expr ExprMap::map(BinaryOpNode &node) {
  return std::make_shared<BinaryOpNode>(
      node.lhs->map(*this), node.rhs->map(*this), node.op, node.loc);
}
Expr ExprMap::map(CallNode &node) {
  for (auto arg : node.args) arg.visit(*this);
  return std::make_shared<CallNode>(node.fun->map(*this), node.args, node.loc);
}
Expr ExprMap::map(IfExprNode &node) {
  return std::make_shared<IfExprNode>(node.cond->map(*this),
                                      node.ifthen->map(*this),
                                      node.ifelse->map(*this), node.loc);
}

}  // namespace sscad
