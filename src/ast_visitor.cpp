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
#include "ast_visitor.h"

namespace sscad {

void AstVisitor::visit(Node &node) {
  if (typeid(node) == typeid(AssignNode)) {
    visit(static_cast<AssignNode &>(node));
  } else if (typeid(node) == typeid(SingleModuleCall)) {
    visit(static_cast<SingleModuleCall &>(node));
  } else if (typeid(node) == typeid(IfModule)) {
    visit(static_cast<IfModule &>(node));
  } else if (typeid(node) == typeid(ModuleModifier)) {
    visit(static_cast<ModuleModifier &>(node));
  } else if (typeid(node) == typeid(ModuleDecl)) {
    visit(static_cast<ModuleDecl &>(node));
  } else if (typeid(node) == typeid(FunctionDecl)) {
    visit(static_cast<FunctionDecl &>(node));
  } else if (typeid(node) == typeid(NumberNode)) {
    visit(static_cast<NumberNode &>(node));
  } else if (typeid(node) == typeid(StringNode)) {
    visit(static_cast<StringNode &>(node));
  } else if (typeid(node) == typeid(UndefNode)) {
    visit(static_cast<UndefNode &>(node));
  } else if (typeid(node) == typeid(IdentNode)) {
    visit(static_cast<IdentNode &>(node));
  } else if (typeid(node) == typeid(UnaryOpNode)) {
    visit(static_cast<UnaryOpNode &>(node));
  } else if (typeid(node) == typeid(BinaryOpNode)) {
    visit(static_cast<BinaryOpNode &>(node));
  } else if (typeid(node) == typeid(CallNode)) {
    visit(static_cast<CallNode &>(node));
  } else if (typeid(node) == typeid(IfExprNode)) {
    visit(static_cast<IfExprNode &>(node));
  } else if (typeid(node) == typeid(ListExprNode)) {
    visit(static_cast<ListExprNode &>(node));
  } else if (typeid(node) == typeid(RangeNode)) {
    visit(static_cast<RangeNode &>(node));
  } else if (typeid(node) == typeid(ListCompNode)) {
    visit(static_cast<ListCompNode &>(node));
  } else if (typeid(node) == typeid(ListCompCNode)) {
    visit(static_cast<ListCompCNode &>(node));
  } else if (typeid(node) == typeid(ListIndexNode)) {
    visit(static_cast<ListIndexNode &>(node));
  } else if (typeid(node) == typeid(LetNode)) {
    visit(static_cast<LetNode &>(node));
  } else if (typeid(node) == typeid(LambdaNode)) {
    visit(static_cast<LambdaNode &>(node));
  } else {
    throw std::runtime_error("unreachable");
  }
}

void AstVisitor::visit(AssignNode &node) {
  if (node.expr != nullptr) visit(node.expr);
}
void AstVisitor::visit(ModuleBody &node) {
  for (auto &assign : node.assignments) visit(assign);
  for (auto &child : node.children) visit(child);
}
void AstVisitor::visit(SingleModuleCall &node) {
  for (auto &arg : node.args) visit(arg);
  visit(node.body);
}
void AstVisitor::visit(IfModule &node) {
  for (auto &arg : node.args) visit(arg);
  visit(node.ifthen);
  visit(node.ifelse);
}
void AstVisitor::visit(ModuleModifier &node) { visit(node.module); }
void AstVisitor::visit(ModuleDecl &node) {
  for (auto &arg : node.args) visit(arg);
  visit(node.body);
}
void AstVisitor::visit(FunctionDecl &node) {
  for (auto &arg : node.args) visit(arg);
  visit(node.body);
}
void AstVisitor::visit(UnaryOpNode &node) { visit(node.operand); }
void AstVisitor::visit(BinaryOpNode &node) {
  visit(node.lhs);
  visit(node.rhs);
}
void AstVisitor::visit(CallNode &node) {
  visit(node.fun);
  for (auto &arg : node.args) visit(arg);
}
void AstVisitor::visit(IfExprNode &node) {
  visit(node.cond);
  visit(node.ifthen);
  visit(node.ifelse);
}
void AstVisitor::visit(ListExprNode &node) {
  for (auto &elem : node.elements) visit(elem.first);
}
void AstVisitor::visit(RangeNode &node) {
  visit(node.start);
  visit(node.step);
  visit(node.end);
}
void AstVisitor::visit(ListCompNode &node) {
  for (auto &assignment : node.assignments) visit(assignment);
  for (auto &tuple : node.generators) {
    visit(std::get<0>(tuple));
    visit(std::get<1>(tuple));
  }
}
void AstVisitor::visit(ListCompCNode &node) {
  for (auto &assignment : node.init) visit(assignment);
  visit(node.cond);
  for (auto &assignment : node.update) visit(assignment);
}
void AstVisitor::visit(ListIndexNode &node) {
  visit(node.list);
  visit(node.index);
}
void AstVisitor::visit(LetNode &node) {
  for (auto &assignment : node.bindings) visit(assignment);
  visit(node.expr);
}
void AstVisitor::visit(LambdaNode &node) {
  for (auto &param : node.params) visit(param);
  visit(node.expr);
}
void AstVisitor::visit(TranslationUnit &unit) {
  for (auto &module : unit.modules) visit(module);
  for (auto &fun : unit.functions) visit(fun);
  for (auto &assign : unit.assignments) visit(assign);
  for (auto &call : unit.moduleCalls) visit(call);
}

void ExprMap::visit(AssignNode &node) {
  if (node.expr != nullptr) node.expr = map(node.expr);
}
Expr ExprMap::map(ExprNode &node) {
  if (typeid(node) == typeid(NumberNode)) {
    return map(static_cast<NumberNode &>(node));
  } else if (typeid(node) == typeid(StringNode)) {
    return map(static_cast<StringNode &>(node));
  } else if (typeid(node) == typeid(UndefNode)) {
    return map(static_cast<UndefNode &>(node));
  } else if (typeid(node) == typeid(IdentNode)) {
    return map(static_cast<IdentNode &>(node));
  } else if (typeid(node) == typeid(UnaryOpNode)) {
    return map(static_cast<UnaryOpNode &>(node));
  } else if (typeid(node) == typeid(BinaryOpNode)) {
    return map(static_cast<BinaryOpNode &>(node));
  } else if (typeid(node) == typeid(CallNode)) {
    return map(static_cast<CallNode &>(node));
  } else if (typeid(node) == typeid(IfExprNode)) {
    return map(static_cast<IfExprNode &>(node));
  } else if (typeid(node) == typeid(ListExprNode)) {
    return map(static_cast<ListExprNode &>(node));
  } else if (typeid(node) == typeid(RangeNode)) {
    return map(static_cast<RangeNode &>(node));
  } else if (typeid(node) == typeid(ListCompNode)) {
    return map(static_cast<ListCompNode &>(node));
  } else if (typeid(node) == typeid(ListCompCNode)) {
    return map(static_cast<ListCompCNode &>(node));
  } else if (typeid(node) == typeid(ListIndexNode)) {
    return map(static_cast<ListIndexNode &>(node));
  } else if (typeid(node) == typeid(LetNode)) {
    return map(static_cast<LetNode &>(node));
  } else if (typeid(node) == typeid(LambdaNode)) {
    return map(static_cast<LambdaNode &>(node));
  } else {
    throw std::runtime_error("unreachable");
  }
}
Expr ExprMap::map(NumberNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(StringNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(UndefNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(IdentNode &node) { return node.shared_from_this(); }
Expr ExprMap::map(UnaryOpNode &node) {
  return std::make_shared<UnaryOpNode>(map(node.operand), node.op, node.loc);
}
Expr ExprMap::map(BinaryOpNode &node) {
  return std::make_shared<BinaryOpNode>(map(node.lhs), map(node.rhs), node.op,
                                        node.loc);
}
Expr ExprMap::map(CallNode &node) {
  std::vector<AssignNode> args;
  for (auto arg : node.args)
    args.push_back(AssignNode(arg.ident, map(arg.expr), arg.loc));
  return std::make_shared<CallNode>(map(node.fun), args, node.loc);
}
Expr ExprMap::map(IfExprNode &node) {
  return std::make_shared<IfExprNode>(map(node.cond), map(node.ifthen),
                                      map(node.ifelse), node.loc);
}
Expr ExprMap::map(ListExprNode &node) {
  std::vector<std::pair<Expr, bool>> elements;
  for (auto elem : node.elements)
    elements.push_back(std::make_pair(map(elem.first), elem.second));
  return std::make_shared<ListExprNode>(elements, node.loc);
}
Expr ExprMap::map(RangeNode &node) {
  return std::make_shared<RangeNode>(map(node.start), map(node.step),
                                     map(node.end), node.loc);
}
Expr ExprMap::map(ListCompNode &node) {
  std::vector<AssignNode> assignments;
  std::vector<std::tuple<Expr, Expr, bool>> generators;
  for (auto &assignment : node.assignments)
    assignments.emplace_back(assignment.ident, map(assignment.expr),
                             assignment.loc);
  for (auto &tuple : node.generators)
    generators.emplace_back(map(std::get<0>(tuple)), map(std::get<1>(tuple)),
                            std::get<2>(tuple));
  return std::make_shared<ListCompNode>(assignments, generators, node.loc);
}
Expr ExprMap::map(ListCompCNode &node) {
  std::vector<AssignNode> init, update;
  std::vector<std::tuple<Expr, Expr, bool>> generators;
  for (auto &assignment : node.init)
    init.emplace_back(assignment.ident, map(assignment.expr), assignment.loc);
  for (auto &assignment : node.update)
    update.emplace_back(assignment.ident, map(assignment.expr), assignment.loc);
  for (auto &tuple : node.generators)
    generators.emplace_back(map(std::get<0>(tuple)), map(std::get<1>(tuple)),
                            std::get<2>(tuple));
  return std::make_shared<ListCompCNode>(init, map(node.cond), update,
                                         generators, node.loc);
}
Expr ExprMap::map(ListIndexNode &node) {
  return std::make_shared<ListIndexNode>(map(node.list), map(node.index),
                                         node.loc);
}
Expr ExprMap::map(LetNode &node) {
  std::vector<AssignNode> bindings;
  for (auto &assignment : node.bindings)
    bindings.emplace_back(assignment.ident, map(assignment.expr),
                          assignment.loc);
  return std::make_shared<LetNode>(bindings, map(node.expr), node.loc);
}
Expr ExprMap::map(LambdaNode &node) {
  std::vector<AssignNode> params;
  for (auto &assignment : node.params)
    params.emplace_back(assignment.ident, map(assignment.expr), assignment.loc);
  return std::make_shared<LambdaNode>(params, map(node.expr), node.loc);
}

}  // namespace sscad
