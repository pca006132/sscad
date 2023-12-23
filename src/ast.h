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
#include <vector>

#include "ast_visitor.h"
#include "location.h"
namespace sscad {

struct Node {
 public:
  Node(Location loc) : loc(loc) {}

  Location loc;

  virtual void visit(AstVisitor &) = 0;
};

struct ExprNode : public Node {
 public:
  ExprNode(Location loc) : Node(loc) {}
};

using Expr = std::shared_ptr<ExprNode>;

struct AssignNode final : public Node {
 public:
  AssignNode() : ident(""), expr(nullptr), Node(Location{}) {}
  AssignNode(const std::string ident, Expr expr, Location loc)
      : ident(ident), expr(expr), Node(loc) {}

  std::string ident;
  Expr expr;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct ModuleCall : public Node {
 public:
  ModuleCall(const std::string name, std::vector<AssignNode> args, Location loc)
      : name(name), args(args), Node(loc) {}

  std::string name;
  // positional arguments have empty names
  std::vector<AssignNode> args;
};

struct ModuleBody {
 public:
  ModuleBody() = default;
  ModuleBody(std::vector<AssignNode> assignments,
             std::vector<std::shared_ptr<ModuleCall>> children)
      : assignments(assignments), children(children) {}

  // list of assignment operations in the children
  std::vector<AssignNode> assignments;
  // list of module calls, note that this includes echo and assert
  std::vector<std::shared_ptr<ModuleCall>> children;

  void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct SingleModuleCall : public ModuleCall {
 public:
  SingleModuleCall(const std::string name, std::vector<AssignNode> args,
                   ModuleBody body, Location loc)
      : body(body), ModuleCall(name, args, loc) {}

  ModuleBody body;
  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

// if else module, then part is stored in ModuleCall::children
struct IfModule : public ModuleCall {
 public:
  IfModule(Expr args, ModuleBody ifthen, ModuleBody ifelse, Location loc)
      : ifthen(ifthen),
        ifelse(ifelse),
        ModuleCall("if", {AssignNode("", args, loc)}, loc){};

  ModuleBody ifthen;
  ModuleBody ifelse;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct ModuleModifier : public ModuleCall {
 public:
  ModuleModifier(std::string modifier, std::shared_ptr<ModuleCall> module,
                 Location loc)
      : modifier(modifier), module(module), ModuleCall(modifier, {}, loc) {}

  std::string modifier;
  std::shared_ptr<ModuleCall> module;
  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct ModuleDecl final : public Node {
 public:
  ModuleDecl(std::string name, std::vector<AssignNode> args, ModuleBody body,
             Location loc)
      : name(name), args(args), body(body), Node(loc) {}

  std::string name;
  // arguments with no default value have expr=nullptr
  std::vector<AssignNode> args;
  ModuleBody body;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

enum class UnaryOp : unsigned char { NEG, NOT };
enum class BinOp : unsigned char {
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  EXP,
  LT,
  LE,
  GT,
  GE,
  EQ,
  NEQ,
  AND,
  OR
};

struct NumberNode final : public ExprNode {
 public:
  NumberNode(double value, Location loc) : value(value), ExprNode(loc) {}

  double value;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct StringNode final : public ExprNode {
 public:
  StringNode(std::string str, Location loc) : str(str), ExprNode(loc) {}

  std::string str;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct UndefNode final : public ExprNode {
 public:
  UndefNode(Location loc) : ExprNode(loc) {}

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct IdentNode : public ExprNode {
 public:
  IdentNode(std::string name, Location loc) : name(name), ExprNode(loc) {}

  std::string name;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct UnaryOpNode final : public ExprNode {
 public:
  UnaryOpNode(Expr operand, UnaryOp op, Location loc)
      : operand(operand), op(op), ExprNode(loc) {}

  Expr operand;
  UnaryOp op;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct BinaryOpNode final : public ExprNode {
 public:
  BinaryOpNode(Expr lhs, Expr rhs, BinOp op, Location loc)
      : lhs(lhs), rhs(rhs), op(op), ExprNode(loc) {}

  Expr lhs;
  Expr rhs;
  BinOp op;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct CallNode final : public ExprNode {
 public:
  CallNode(Expr fun, std::vector<AssignNode> args, Location loc)
      : fun(fun), args(args), ExprNode(loc) {}

  Expr fun;
  // positional arguments have empty names
  std::vector<AssignNode> args;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};

struct CondNode final : public ExprNode {
 public:
  CondNode(Expr cond, Expr ifthen, Expr ifelse, Location loc)
      : cond(cond), ifthen(ifthen), ifelse(ifelse), ExprNode(loc) {}

  Expr cond;
  Expr ifthen;
  Expr ifelse;

  virtual void visit(AstVisitor &visitor) { visitor.visit(*this); }
};
// TODO: list, index, list comprehension, lambda, etc.
}  // namespace sscad
