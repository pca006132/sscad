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
  return os << "(" << loc.begin << "," << loc.end << ")";
}

class AstPrinter : public AstVisitor {
 public:
  AstPrinter(std::ostream *ostream) : ostream(ostream) {}

  void visit(AssignNode &assign) override {
    *ostream << "Assign(" << assign.ident << ", ";
    if (assign.expr != nullptr)
      assign.expr->visit(*this);
    else
      *ostream << "undef";
    *ostream << ", loc=" << assign.loc << ")";
  }
  void visit(ModuleBody &body) override {
    *ostream << "[";
    bool start = true;
    for (const auto assign : body.assignments) {
      if (!start) *ostream << ",";
      start = false;
      *ostream << assign.ident << " = ";
      assign.expr->visit(*this);
    }
    for (const auto child : body.children) {
      if (!start) *ostream << ",";
      start = false;
      child->visit(*this);
    }
    *ostream << "]";
  }
  void visit(SingleModuleCall &single) override {
    *ostream << "ModuleCall(" << single.name << ", "
             << "args=(";
    bool start = true;
    for (const auto &assign : single.args) {
      if (!start) *ostream << ",";
      start = false;
      if (!assign.ident.empty()) *ostream << assign.ident << "=";
      assign.expr->visit(*this);
    }
    *ostream << "), ";
    if (!single.body.children.empty() || !single.body.assignments.empty()) {
      *ostream << "body=";
      single.body.visit(*this);
      *ostream << ", ";
    }
    *ostream << "loc=" << single.loc << ")";
  }
  void visit(IfModule &ifmodule) override {
    *ostream << "If(cond=";
    ifmodule.args[0].expr->visit(*this);
    *ostream << ", then=";
    ifmodule.ifthen.visit(*this);
    if (!ifmodule.ifelse.children.empty()) {
      *ostream << ", else=";
      ifmodule.ifelse.visit(*this);
    }
    *ostream << ", loc=" << ifmodule.loc << ")";
  }
  void visit(ModuleModifier &modifier) override {
    *ostream << modifier.modifier;
    modifier.module->visit(*this);
  }
  void visit(NumberNode &number) override {
    *ostream << "Number(" << number.value << ", loc=" << number.loc << ")";
  }
  void visit(StringNode &str) override {
    *ostream << "String(\"";
    // handle escaping
    for (const char c : str.str) {
      switch (c) {
        case '\r':
          *ostream << "\\r";
          break;
        case '\n':
          *ostream << "\\n";
          break;
        case '"':
          *ostream << "\\\"";
          break;
        case '\\':
          *ostream << "\\\\";
          break;
        default:
          *ostream << c;
          break;
      }
    }
    *ostream << "\", loc=" << str.loc << ")";
  }
  void visit(UndefNode &undef) override {
    *ostream << "Undef(loc=" << undef.loc << ")";
  }
  void visit(IdentNode &ident) override {
    *ostream << "Ident(" << ident.name << ", loc=" << ident.loc << ")";
  }
  void visit(UnaryOpNode &unary) override {
    *ostream << "Unary(" << unary.op << ", ";
    unary.operand->visit(*this);
    *ostream << ", loc=" << unary.loc << ")";
  }
  void visit(BinaryOpNode &binary) override {
    *ostream << "Binary(" << binary.op << ", ";
    binary.lhs->visit(*this);
    *ostream << ", ";
    binary.rhs->visit(*this);
    *ostream << ", loc=" << binary.loc << ")";
  }
  void visit(CallNode &call) override {
    *ostream << "Call(";
    call.fun->visit(*this);
    *ostream << ", args=(";
    bool start = true;
    for (const auto &assign : call.args) {
      if (!start) *ostream << ",";
      start = false;
      if (!assign.ident.empty()) *ostream << assign.ident << "=";
      assign.expr->visit(*this);
    }
    *ostream << "), loc=" << call.loc << ")";
  }
  void visit(TranslationUnit &unit) {
    for (auto &assign : unit.assignments) {
      visit(assign);
      *ostream << std::endl;
    }
    for (auto &call : unit.moduleCalls) {
      call->visit(*this);
      *ostream << std::endl;
    }
  }

 private:
  std::ostream *ostream;
};

int main() {
  auto printer = AstPrinter(&std::cout);
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
                 "*if (1+1==2) cube();\n"
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
    printer.visit(fe.parse(0));
    std::cout << "===================" << std::endl;
    printer.visit(fe.parse(1));
    std::cout << "===================" << std::endl;
    printer.visit(fe.parse(2));
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
