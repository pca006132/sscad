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
#include <sstream>
#include <vector>

#include "location.h"
namespace sscad {

class Node {
 public:
  Node(Location loc) : loc(loc) {}

  Location loc;

  virtual void repr(std::stringstream &ss) const = 0;
};

class ExprNode : public Node {
 public:
  ExprNode(Location loc) : Node(loc) {}

  virtual void repr(std::stringstream &ss) const = 0;
};

using Expr = std::shared_ptr<ExprNode>;

class AssignNode final : public Node {
 public:
  AssignNode() : ident(""), expr(nullptr), Node(Location{}) {}
  AssignNode(const std::string ident, Expr expr, Location loc)
      : ident(ident), expr(expr), Node(loc) {}

  std::string ident;
  Expr expr;

  virtual void repr(std::stringstream &ss) const override;
};

class ModuleCall final : public Node {
 public:
  ModuleCall(const std::string name, std::vector<AssignNode> args,
             std::vector<std::shared_ptr<Node>> children, Location loc)
      : name(name), args(args), children(children), Node(loc) {}

  std::string name;
  std::vector<AssignNode> args;
  std::vector<std::shared_ptr<Node>> children;

  virtual void repr(std::stringstream &ss) const override;
};

class ModuleDecl final : public Node {
 public:
  ModuleDecl(std::string name, std::vector<AssignNode> args,
             std::vector<std::shared_ptr<Node>> body, Location loc)
      : name(name), args(args), body(body), Node(loc) {}

  std::string name;
  std::vector<AssignNode> args;
  std::vector<std::shared_ptr<Node>> body;

  virtual void repr(std::stringstream &ss) const override;
};

enum UnaryOp { NEG, NOT };
enum BinOp { ADD, SUB, MUL, DIV, MOD, EXP, LT, LE, GT, GE, EQ, NEQ, AND, OR };

void UnaryToString(std::stringstream &ss, UnaryOp op);
void BinaryToString(std::stringstream &ss, BinOp op);

class NumberNode : public ExprNode {
 public:
  NumberNode(double value, Location loc) : value(value), ExprNode(loc) {}

  double value;

  virtual void repr(std::stringstream &ss) const override { ss << value; }
};

class StringNode : public ExprNode {
 public:
  StringNode(std::string str, Location loc) : str(str), ExprNode(loc) {}

  std::string str;

  virtual void repr(std::stringstream &ss) const override {
    // think about escaping later
    ss << '"' << str << '"';
  }
};

class UndefNode : public ExprNode {
 public:
  UndefNode(Location loc) : ExprNode(loc) {}

  virtual void repr(std::stringstream &ss) const override { ss << "undef"; }
};

class IdentNode : public ExprNode {
 public:
  IdentNode(std::string name, Location loc) : name(name), ExprNode(loc) {}

  std::string name;

  virtual void repr(std::stringstream &ss) const override { ss << name; }
};

class UnaryOpNode : public ExprNode {
 public:
  UnaryOpNode(Expr operand, UnaryOp op, Location loc)
      : operand(operand), op(op), ExprNode(loc) {}

  Expr operand;
  UnaryOp op;

  virtual void repr(std::stringstream &ss) const override {
    ss << "(";
    sscad::UnaryToString(ss, op);
    operand->repr(ss);
    ss << ")";
  }
};

class BinaryOpNode : public ExprNode {
 public:
  BinaryOpNode(Expr lhs, Expr rhs, BinOp op, Location loc)
      : lhs(lhs), rhs(rhs), op(op), ExprNode(loc) {}

  Expr lhs;
  Expr rhs;
  BinOp op;

  virtual void repr(std::stringstream &ss) const override {
    ss << "(";
    lhs->repr(ss);
    sscad::BinaryToString(ss, op);
    rhs->repr(ss);
    ss << ")";
  }
};

class CallNode : public ExprNode {
 public:
  CallNode(Expr fun, std::vector<AssignNode> args, Location loc)
      : fun(fun), args(args), ExprNode(loc) {}

  Expr fun;
  std::vector<AssignNode> args;

  virtual void repr(std::stringstream &ss) const override;
};
// TODO: list, index, list comprehension, let, cond, lambda, string, etc.
}  // namespace sscad
