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

enum class UnaryOp : unsigned char { NEG, NOT };
// clang-format off
enum class BinOp : unsigned char {
  ADD, SUB, MUL, DIV, MOD, EXP,
  LT, LE, GT, GE, EQ, NEQ, AND, OR };
// clang-format on

struct Node {
 public:
  Node(Location loc) : loc(loc) {}

  Location loc;

  virtual void visit(AstVisitor &) = 0;
};

struct ExprNode : public Node {
 public:
  ExprNode(Location loc) : Node(loc) {}

  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) = 0;
  virtual bool isConstValue() { return false; }
};

using Expr = std::shared_ptr<ExprNode>;

struct AssignNode final : public Node {
 public:
  AssignNode() : ident(""), expr(nullptr), Node(Location{}) {}
  AssignNode(const std::string ident, Expr expr, Location loc)
      : ident(ident), expr(expr), Node(loc) {}

  std::string ident;
  Expr expr;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
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
  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
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

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
};

struct ModuleModifier : public ModuleCall {
 public:
  ModuleModifier(std::string modifier, std::shared_ptr<ModuleCall> module,
                 Location loc)
      : modifier(modifier), module(module), ModuleCall(modifier, {}, loc) {}

  std::string modifier;
  std::shared_ptr<ModuleCall> module;
  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
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

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
};

struct FunctionDecl final : public Node {
 public:
  FunctionDecl(std::string name, std::vector<AssignNode> args, Expr body,
               Location loc)
      : name(name), args(args), body(body), Node(loc) {}

  std::string name;
  // arguments with no default value have expr=nullptr
  std::vector<AssignNode> args;
  Expr body;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
};

struct NumberNode final : public ExprNode,
                          std::enable_shared_from_this<NumberNode> {
 public:
  NumberNode(double value, Location loc) : value(value), ExprNode(loc) {}

  double value;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
  virtual bool isConstValue() override { return true; }
};

struct StringNode final : public ExprNode,
                          std::enable_shared_from_this<StringNode> {
 public:
  StringNode(std::string str, Location loc) : str(str), ExprNode(loc) {}

  std::string str;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
  virtual bool isConstValue() override { return true; }
};

struct UndefNode final : public ExprNode,
                         public std::enable_shared_from_this<UndefNode> {
 public:
  UndefNode(Location loc) : ExprNode(loc) {}

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
  virtual bool isConstValue() override { return true; }
};

struct IdentNode : public ExprNode, std::enable_shared_from_this<IdentNode> {
 public:
  IdentNode(std::string name, Location loc) : name(name), ExprNode(loc) {}

  std::string name;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
  bool isConfigVar() const { return name.length() > 1 && name[0] == '$'; }
};

struct UnaryOpNode final : public ExprNode,
                           std::enable_shared_from_this<UnaryOpNode> {
 public:
  UnaryOpNode(Expr operand, UnaryOp op, Location loc)
      : operand(operand), op(op), ExprNode(loc) {}

  Expr operand;
  UnaryOp op;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
};

struct BinaryOpNode final : public ExprNode,
                            std::enable_shared_from_this<BinaryOpNode> {
 public:
  BinaryOpNode(Expr lhs, Expr rhs, BinOp op, Location loc)
      : lhs(lhs), rhs(rhs), op(op), ExprNode(loc) {}

  Expr lhs;
  Expr rhs;
  BinOp op;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
};

struct CallNode final : public ExprNode,
                        std::enable_shared_from_this<CallNode> {
 public:
  CallNode(Expr fun, std::vector<AssignNode> args, Location loc)
      : fun(fun), args(args), ExprNode(loc) {}

  Expr fun;
  // positional arguments have empty names
  std::vector<AssignNode> args;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
};

struct IfExprNode final : public ExprNode,
                          std::enable_shared_from_this<IfExprNode> {
 public:
  IfExprNode(Expr cond, Expr ifthen, Expr ifelse, Location loc)
      : cond(cond), ifthen(ifthen), ifelse(ifelse), ExprNode(loc) {}

  Expr cond;
  Expr ifthen;
  Expr ifelse;

  virtual void visit(AstVisitor &visitor) override { visitor.visit(*this); }
  virtual std::shared_ptr<ExprNode> map(ExprMap &mapper) override {
    return mapper.map(*this);
  }
};
// TODO: list, index, list comprehension, lambda, etc.
}  // namespace sscad
