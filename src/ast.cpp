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

#include "ast.h"
namespace sscad {

template <typename T>
void repr(std::stringstream &ss, std::shared_ptr<T> v) {
  v->repr(ss);
}

template <typename T>
void repr(std::stringstream &ss, const T &v) {
  v.repr(ss);
}

template <typename I>
void listrepr(std::stringstream &ss, std::string delimiter, const I &iterable) {
  bool first = true;
  for (const auto &iter : iterable) {
    if (first)
      first = false;
    else
      ss << delimiter;
    repr(ss, iter);
  }
}

void UnaryToString(std::stringstream &ss, UnaryOp op) {
  switch (op) {
    case UnaryOp::NEG:
      ss << "-";
      break;
    case UnaryOp::NOT:
      ss << "!";
      break;
  }
}

void BinaryToString(std::stringstream &ss, BinOp op) {
  switch (op) {
    case BinOp::ADD:
      ss << "+";
      break;
    case BinOp::SUB:
      ss << "-";
      break;
    case BinOp::MUL:
      ss << "*";
      break;
    case BinOp::DIV:
      ss << "/";
      break;
    case BinOp::MOD:
      ss << "%";
      break;
    case BinOp::EXP:
      ss << "^";
      break;
    case BinOp::LT:
      ss << "<";
      break;
    case BinOp::LE:
      ss << "<=";
      break;
    case BinOp::GT:
      ss << ">";
      break;
    case BinOp::GE:
      ss << ">=";
      break;
    case BinOp::EQ:
      ss << "=";
      break;
    case BinOp::NEQ:
      ss << "!=";
      break;
    case BinOp::AND:
      ss << "&&";
      break;
    case BinOp::OR:
      ss << "||";
      break;
  }
}

void AssignNode::repr(std::stringstream &ss) const {
  ss << ident << " = ";
  if (expr) expr->repr(ss);
}

void CallNode::repr(std::stringstream &ss) const {
  fun->repr(ss);
  ss << "(";
  listrepr(ss, ", ", args);
  ss << ")";
}
}  // namespace sscad
