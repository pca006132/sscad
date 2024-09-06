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
  virtual void dummy() {}
};

struct ExprNode : public Node {
 public:
  ExprNode(Location loc) : Node(loc) {}

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
};

// for and intersection_for are represented as builtin SingleModuleCall
struct SingleModuleCall : public ModuleCall {
 public:
  SingleModuleCall(const std::string name, std::vector<AssignNode> args,
                   ModuleBody body, Location loc)
      : body(body), ModuleCall(name, args, loc) {}

  ModuleBody body;
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
};

struct ModuleModifier : public ModuleCall {
 public:
  ModuleModifier(std::string modifier, std::shared_ptr<ModuleCall> module,
                 Location loc)
      : modifier(modifier), module(module), ModuleCall(modifier, {}, loc) {}

  std::string modifier;
  std::shared_ptr<ModuleCall> module;
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
};

struct NumberNode final : public ExprNode,
                          std::enable_shared_from_this<NumberNode> {
 public:
  NumberNode(double value, Location loc) : value(value), ExprNode(loc) {}

  double value;

  virtual bool isConstValue() override { return true; }
};

struct StringNode final : public ExprNode,
                          std::enable_shared_from_this<StringNode> {
 public:
  StringNode(std::string str, Location loc) : str(str), ExprNode(loc) {}

  std::string str;

  virtual bool isConstValue() override { return true; }
};

struct UndefNode final : public ExprNode,
                         public std::enable_shared_from_this<UndefNode> {
 public:
  UndefNode(Location loc) : ExprNode(loc) {}

  virtual bool isConstValue() override { return true; }
};

struct IdentNode : public ExprNode, std::enable_shared_from_this<IdentNode> {
 public:
  IdentNode(std::string name, Location loc) : name(name), ExprNode(loc) {}

  std::string name;

  bool isConfigVar() const { return name.length() > 1 && name[0] == '$'; }
};

struct UnaryOpNode final : public ExprNode,
                           std::enable_shared_from_this<UnaryOpNode> {
 public:
  UnaryOpNode(Expr operand, UnaryOp op, Location loc)
      : operand(operand), op(op), ExprNode(loc) {}

  Expr operand;
  UnaryOp op;
};

struct BinaryOpNode final : public ExprNode,
                            std::enable_shared_from_this<BinaryOpNode> {
 public:
  BinaryOpNode(Expr lhs, Expr rhs, BinOp op, Location loc)
      : lhs(lhs), rhs(rhs), op(op), ExprNode(loc) {}

  Expr lhs;
  Expr rhs;
  BinOp op;
};

struct CallNode final : public ExprNode,
                        std::enable_shared_from_this<CallNode> {
 public:
  CallNode(Expr fun, std::vector<AssignNode> args, Location loc)
      : fun(fun), args(args), ExprNode(loc) {}

  Expr fun;
  // positional arguments have empty names
  std::vector<AssignNode> args;
};

struct IfExprNode final : public ExprNode,
                          std::enable_shared_from_this<IfExprNode> {
 public:
  IfExprNode(Expr cond, Expr ifthen, Expr ifelse, Location loc)
      : cond(cond), ifthen(ifthen), ifelse(ifelse), ExprNode(loc) {}

  Expr cond;
  Expr ifthen;
  Expr ifelse;
};

struct ListExprNode final : public ExprNode,
                            std::enable_shared_from_this<ListExprNode> {
 public:
  ListExprNode(std::vector<std::pair<Expr, bool>> elements, Location loc)
      : elements(elements), ExprNode(loc) {}

  // element, is_each pair
  // if is_each is true, the element should be flattened
  std::vector<std::pair<Expr, bool>> elements;
};

struct RangeNode final : public ExprNode,
                         std::enable_shared_from_this<RangeNode> {
 public:
  RangeNode(Expr start, Expr step, Expr end, Location loc)
      : start(start), step(step), end(end), ExprNode(loc) {}

  Expr start, step, end;
};

// TODO: multiple generator expression is encoded as nested comprehension with
// is_each set to true
struct ListCompNode final : public ExprNode,
                            std::enable_shared_from_this<ListCompNode> {
 public:
  ListCompNode(std::vector<AssignNode> assignments,
               std::vector<std::tuple<Expr, Expr, bool>> generators,
               Location loc)
      : assignments(assignments), generators(generators), ExprNode(loc) {}

  // iterator variables
  std::vector<AssignNode> assignments;
  // condition, element, is_each triple
  std::vector<std::tuple<Expr, Expr, bool>> generators;
};

struct ListCompCNode final : public ExprNode,
                             std::enable_shared_from_this<ListCompCNode> {
 public:
  ListCompCNode(std::vector<AssignNode> init, Expr cond,
                std::vector<AssignNode> update,
                std::vector<std::tuple<Expr, Expr, bool>> generators,
                Location loc)
      : init(init),
        cond(cond),
        update(update),
        generators(generators),
        ExprNode(loc) {}
  std::vector<AssignNode> init;
  Expr cond;
  std::vector<AssignNode> update;
  // condition, element, is_each triple
  std::vector<std::tuple<Expr, Expr, bool>> generators;
};

struct ListIndexNode final : public ExprNode,
                             std::enable_shared_from_this<ListIndexNode> {
 public:
  ListIndexNode(Expr list, Expr index, Location loc)
      : list(list), index(index), ExprNode(loc) {}
  Expr list;
  Expr index;
};

struct LetNode final : public ExprNode, std::enable_shared_from_this<LetNode> {
 public:
  LetNode(std::vector<AssignNode> bindings, Expr expr, Location loc)
      : bindings(bindings), expr(expr), ExprNode(loc) {}
  std::vector<AssignNode> bindings;
  Expr expr;
};

// closure is handled by the bytecode generator
struct LambdaNode final : public ExprNode,
                          std::enable_shared_from_this<LambdaNode> {
 public:
  LambdaNode(std::vector<AssignNode> params, Expr expr, Location loc)
      : params(params), expr(expr), ExprNode(loc) {}
  std::vector<AssignNode> params;
  Expr expr;
};
}  // namespace sscad
