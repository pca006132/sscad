/**
 * Copyright (C) 2023 The sscad Authors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <functional>
#include <sstream>
#include <vector>

#include "location.h"
namespace sscad {

class Node {
 public:
  Node(Location loc) : loc(loc) {}

  Location loc;

  virtual void visit(std::function<void(Node *)> &f) = 0;
};

class ExprNode : public Node {
 public:
  ExprNode(Location loc) : Node(loc) {}
};

using Expr = std::shared_ptr<ExprNode>;

class AssignNode final : public Node {
 public:
  AssignNode() : ident(""), expr(nullptr), Node(Location{}) {}
  AssignNode(const std::string ident, Expr expr, Location loc)
      : ident(ident), expr(expr), Node(loc) {}

  std::string ident;
  Expr expr;

  virtual void visit(std::function<void(Node *)> &f) override {
    if (expr != nullptr) f(expr.get());
  }
};

class ModuleCall : public Node {
 public:
  ModuleCall(const std::string name, std::vector<AssignNode> args, Location loc)
      : name(name), args(args), Node(loc) {}

  std::string name;
  // positional arguments have empty names
  std::vector<AssignNode> args;
};

class ModuleBody {
 public:
  ModuleBody(std::vector<AssignNode> assignments,
             std::vector<std::shared_ptr<ModuleCall>> children)
      : assignments(assignments), children(children) {}

  // list of assignment operations in the children
  std::vector<AssignNode> assignments;
  // list of module calls, note that this includes echo and assert
  std::vector<std::shared_ptr<ModuleCall>> children;

  void repr(std::stringstream &ss) const;
  void visit(std::function<void(Node *)> &f) {
    for (auto &iter : assignments) f(&iter);
    for (auto &iter : children) f(iter.get());
  }
};

class SingleModuleCall : public ModuleCall {
 public:
  SingleModuleCall(const std::string name, std::vector<AssignNode> args,
                   ModuleBody body, Location loc)
      : body(body), ModuleCall(name, args, loc) {}

  ModuleBody body;
  virtual void visit(std::function<void(Node *)> &f) override {
    for (auto &iter : args) f(&iter);
    body.visit(f);
  }
};

// if else module, then part is stored in ModuleCall::children
class IfModule : public ModuleCall {
 public:
  IfModule(std::vector<AssignNode> args, ModuleBody ifthen, ModuleBody ifelse,
           Location loc)
      : ifthen(ifthen), ifelse(ifelse), ModuleCall("for", args, loc){};

  ModuleBody ifthen;
  ModuleBody ifelse;

  virtual void visit(std::function<void(Node *)> &f) override {
    for (auto &iter : args) f(&iter);
    ifthen.visit(f);
    ifelse.visit(f);
  }
};

class ModuleDecl final : public Node {
 public:
  ModuleDecl(std::string name, std::vector<AssignNode> args, ModuleBody body,
             Location loc)
      : name(name), args(args), body(body), Node(loc) {}

  std::string name;
  // arguments with no default value have expr=nullptr
  std::vector<AssignNode> args;
  ModuleBody body;

  virtual void visit(std::function<void(Node *)> &f) override {
    for (auto &iter : args) f(&iter);
    body.visit(f);
  }
};

enum UnaryOp { NEG, NOT };
enum BinOp { ADD, SUB, MUL, DIV, MOD, EXP, LT, LE, GT, GE, EQ, NEQ, AND, OR };

void UnaryToString(std::stringstream &ss, UnaryOp op);
void BinaryToString(std::stringstream &ss, BinOp op);

class NumberNode final : public ExprNode {
 public:
  NumberNode(double value, Location loc) : value(value), ExprNode(loc) {}

  double value;

  virtual void visit(std::function<void(Node *)> &f) override {}
};

class StringNode final : public ExprNode {
 public:
  StringNode(std::string str, Location loc) : str(str), ExprNode(loc) {}

  std::string str;

  virtual void visit(std::function<void(Node *)> &f) override {}
};

class UndefNode final : public ExprNode {
 public:
  UndefNode(Location loc) : ExprNode(loc) {}

  virtual void visit(std::function<void(Node *)> &f) override {}
};

class IdentNode : public ExprNode {
 public:
  IdentNode(std::string name, Location loc) : name(name), ExprNode(loc) {}

  std::string name;

  virtual void visit(std::function<void(Node *)> &f) override {}
};

class UnaryOpNode final : public ExprNode {
 public:
  UnaryOpNode(Expr operand, UnaryOp op, Location loc)
      : operand(operand), op(op), ExprNode(loc) {}

  Expr operand;
  UnaryOp op;

  virtual void visit(std::function<void(Node *)> &f) override {
    f(operand.get());
  }
};

class BinaryOpNode final : public ExprNode {
 public:
  BinaryOpNode(Expr lhs, Expr rhs, BinOp op, Location loc)
      : lhs(lhs), rhs(rhs), op(op), ExprNode(loc) {}

  Expr lhs;
  Expr rhs;
  BinOp op;

  virtual void visit(std::function<void(Node *)> &f) override {
    f(lhs.get());
    f(rhs.get());
  }
};

class CallNode final : public ExprNode {
 public:
  CallNode(Expr fun, std::vector<AssignNode> args, Location loc)
      : fun(fun), args(args), ExprNode(loc) {}

  Expr fun;
  // positional arguments have empty names
  std::vector<AssignNode> args;

  virtual void visit(std::function<void(Node *)> &f) override {
    f(fun.get());
    for (auto &iter : args) f(&iter);
  }
};
// TODO: list, index, list comprehension, cond, lambda, etc.
}  // namespace sscad
