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

#include <iostream>
#include <sstream>

#include "ast.h"
#include "frontend.h"

using namespace sscad;
using namespace std::string_literals;

std::ostream &operator<<(std::ostream &os, UnaryOp op) {
  switch (op) {
    case UnaryOp::NEG:
      os << "-";
      break;
    case UnaryOp::NOT:
      os << "!";
      break;
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, BinOp op) {
  switch (op) {
    case BinOp::ADD:
      os << "+";
      break;
    case BinOp::SUB:
      os << "-";
      break;
    case BinOp::MUL:
      os << "*";
      break;
    case BinOp::DIV:
      os << "/";
      break;
    case BinOp::MOD:
      os << "%";
      break;
    case BinOp::EXP:
      os << "^";
      break;
    case BinOp::LT:
      os << "<";
      break;
    case BinOp::LE:
      os << "<=";
      break;
    case BinOp::GT:
      os << ">";
      break;
    case BinOp::GE:
      os << ">=";
      break;
    case BinOp::EQ:
      os << "=";
      break;
    case BinOp::NEQ:
      os << "!=";
      break;
    case BinOp::AND:
      os << "&&";
      break;
    case BinOp::OR:
      os << "||";
      break;
  }
  return os;
}

inline std::ostream &operator<<(std::ostream &os,
                                const Location::Position &pos) {
  return os << pos.src << ":" << pos.line << ":" << pos.column;
}

inline std::ostream &operator<<(std::ostream &os, const Location &loc) {
  return os << "L(" << loc.begin << "," << loc.end << ")";
}

void printBody(const ModuleBody &body);

void printAST(const Node *node) {
  std::function<void(const Node *)> f = printAST;
  if (const auto assign = dynamic_cast<const AssignNode *>(node)) {
    std::cout << "Assign(" << assign->ident << ", ";
    if (assign->expr)
      f(assign->expr.get());
    else
      std::cout << "undef";
    std::cout << ", loc=" << assign->loc << ")";
  } else if (const auto single = dynamic_cast<const SingleModuleCall *>(node)) {
    std::cout << "ModuleCall(" << single->name << ", "
              << "args=(";
    for (const auto &assign : single->args) {
      if (!assign.ident.empty()) std::cout << assign.ident << "=";
      f(assign.expr.get());
      std::cout << ",";
    }
    std::cout << "), loc=" << single->loc << ")";
    printBody(single->body);
  } else if (const auto ifmodule = dynamic_cast<const IfModule *>(node)) {
    std::cout << "If(cond=";
    f(ifmodule->args[0].expr.get());
    std::cout << ", then=";
    printBody(ifmodule->ifthen);
    if (!ifmodule->ifelse.children.empty()) {
      std::cout << ", else=";
      printBody(ifmodule->ifelse);
    }
    std::cout << ", loc=" << ifmodule->loc << ")";
  } else if (const auto modifier = dynamic_cast<const ModuleModifier *>(node)) {
    std::cout << modifier->modifier;
    printAST(modifier->module.get());
  } else if (const auto number = dynamic_cast<const NumberNode *>(node)) {
    std::cout << "Number(" << number->value << ", loc=" << number->loc << ")";
  } else if (const auto str = dynamic_cast<const StringNode *>(node)) {
    std::cout << "String(\"";
    // handle escaping
    for (const char c : str->str) {
      switch (c) {
        case '\r':
          std::cout << "\\r";
          break;
        case '\n':
          std::cout << "\\n";
          break;
        case '"':
          std::cout << "\\\"";
          break;
        case '\\':
          std::cout << "\\\\";
          break;
        default:
          std::cout << c;
          break;
      }
    }
    std::cout << "\", loc=" << str->loc << ")";
  } else if (dynamic_cast<const UndefNode *>(node)) {
    std::cout << "undef";
  } else if (const auto ident = dynamic_cast<const IdentNode *>(node)) {
    std::cout << "Ident(" << ident->name << ", loc=" << ident->loc << ")";
  } else if (const auto unary = dynamic_cast<const UnaryOpNode *>(node)) {
    std::cout << "Unary(" << unary->op << ", ";
    f(unary->operand.get());
    std::cout << ", loc=" << unary->loc << ")";
  } else if (const auto binary = dynamic_cast<const BinaryOpNode *>(node)) {
    std::cout << "Binary(" << binary->op << ", ";
    f(binary->lhs.get());
    std::cout << ", ";
    f(binary->rhs.get());
    std::cout << ", loc=" << binary->loc << ")";
  } else if (const auto call = dynamic_cast<const CallNode *>(node)) {
    std::cout << "Call(";
    f(call->fun.get());
    std::cout << ", args=(";
    for (const auto &assign : call->args) {
      if (!assign.ident.empty()) std::cout << assign.ident << "=";
      f(assign.expr.get());
      std::cout << ",";
    }
    std::cout << "), loc=" << call->loc << ")";
  } else {
    std::cout << "???";
  }
}

void printBody(const ModuleBody &body) {
  if (body.assignments.empty() && body.children.empty()) {
    std::cout << ";";
  } else if (body.assignments.empty() && body.children.size() == 1) {
    printAST(body.children[0].get());
    std::cout << ";";
  } else {
    std::cout << "{" << std::endl;
    for (const auto assign : body.assignments) {
      std::cout << assign.ident << " = ";
      printAST(assign.expr.get());
      std::cout << ";";
    }
    for (const auto child : body.children) {
      printAST(child.get());
      std::cout << ";";
    }
    std::cout << "}" << std::endl;
  }
}

void printUnit(const TranslationUnit &unit) {
  for (const auto &assign : unit.assignments) {
    printAST(&assign);
    std::cout << std::endl;
  }
  for (const auto &call : unit.moduleCalls) {
    printAST(call.get());
    std::cout << std::endl;
  }
}

int main() {
  auto resolver = [](std::string name, sscad::FileHandle src) {
    if (name == "a") return 0;
    if (name == "b") return 1;
    if (name == "c") return 2;
    throw std::runtime_error("unknown file "s + name);
  };
  auto provider = [](sscad::FileHandle src) {
    auto ss = std::make_unique<std::stringstream>();
    switch (src) {
      case 0:
        (*ss) << "echo(a + b(123, c = 456));";
        break;
      case 1:
        (*ss) << "include<a>\n"
                 "echo(foo + naïve);\n"
                 "foo2(123) { cube(); }\n"
                 "module foo2(a, b = 2) { cube(); children(); }\n"
                 "if (1+1==2) cube();\n"
                 "if (1+1==2) { a(); } else { b(); }";
        break;
      case 2:
        (*ss) << "echo(a * b + c * d > 12 && foo ^ bar);\r\n"
                 "echo(a+b+c\n+d);\nfoo@";
        break;
    }
    return ss;
  };

  Frontend fe(resolver, provider);
  try {
    printUnit(fe.parse(0));
    std::cout << "===================" << std::endl;
    printUnit(fe.parse(1));
    std::cout << "===================" << std::endl;
    printUnit(fe.parse(2));
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
