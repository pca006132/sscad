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
#include "ast_printer.h"

#include "ast.h"
#include "frontend.h"
namespace sscad {
void AstPrinter::visit(AssignNode &assign) {
  *ostream << "Assign(" << assign.ident << ", ";
  if (assign.expr != nullptr)
    assign.expr->visit(*this);
  else
    *ostream << "undef";
  *ostream << ", loc=" << assign.loc << ")";
}

void AstPrinter::visit(ModuleBody &body) {
  *ostream << "[";
  bool start = true;
  for (const auto &assign : body.assignments) {
    if (!start) *ostream << ",";
    start = false;
    *ostream << assign.ident << " = ";
    assign.expr->visit(*this);
  }
  for (const auto &child : body.children) {
    if (!start) *ostream << ",";
    start = false;
    child->visit(*this);
  }
  *ostream << "]";
}

void AstPrinter::visit(SingleModuleCall &single) {
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

void AstPrinter::visit(IfModule &ifmodule) {
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

void AstPrinter::visit(ModuleModifier &modifier) {
  *ostream << modifier.modifier;
  modifier.module->visit(*this);
}

void AstPrinter::visit(ModuleDecl &module) {
  *ostream << "Module(" << module.name << ", args=(";
  bool start = true;
  for (const auto &assign : module.args) {
    if (!start) *ostream << ",";
    start = false;
    *ostream << assign.ident << "=";
    if (assign.expr != nullptr) assign.expr->visit(*this);
  }
  *ostream << "), ";
  visit(module.body);
  *ostream << ", loc=" << module.loc << ")";
}

void AstPrinter::visit(FunctionDecl &fun) {
  *ostream << "Function(" << fun.name << ", ";
  bool start = true;
  for (const auto &assign : fun.args) {
    if (!start) *ostream << ",";
    start = false;
    *ostream << assign.ident << "=";
    if (assign.expr != nullptr) assign.expr->visit(*this);
  }
  *ostream << "), ";
  fun.body->visit(*this);
  *ostream << ", loc=" << fun.loc << ")";
}

void AstPrinter::visit(NumberNode &number) {
  *ostream << "Number(" << number.value << ", loc=" << number.loc << ")";
}

void AstPrinter::visit(StringNode &str) {
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

void AstPrinter::visit(UndefNode &undef) {
  *ostream << "Undef(loc=" << undef.loc << ")";
}

void AstPrinter::visit(IdentNode &ident) {
  *ostream << "Ident(" << ident.name << ", loc=" << ident.loc << ")";
}

void AstPrinter::visit(UnaryOpNode &unary) {
  *ostream << "Unary(" << unary.op << ", ";
  unary.operand->visit(*this);
  *ostream << ", loc=" << unary.loc << ")";
}

void AstPrinter::visit(BinaryOpNode &binary) {
  *ostream << "Binary(" << binary.op << ", ";
  binary.lhs->visit(*this);
  *ostream << ", ";
  binary.rhs->visit(*this);
  *ostream << ", loc=" << binary.loc << ")";
}

void AstPrinter::visit(CallNode &call) {
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

void AstPrinter::visit(IfExprNode &ifcond) {
  *ostream << "IfExpr(cond=";
  ifcond.cond->visit(*this);
  *ostream << ", then=";
  ifcond.ifthen->visit(*this);
  if (ifcond.ifelse != nullptr) {
    *ostream << ", else=";
    ifcond.ifelse->visit(*this);
  }

  *ostream << ", loc=" << ifcond.loc << ")";
}

void AstPrinter::visit(TranslationUnit &unit) {
  *ostream << "modules:" << std::endl;
  for (auto &module : unit.modules) {
    visit(module);
    *ostream << std::endl;
  }
  *ostream << "functions:" << std::endl;
  for (auto &fun : unit.functions) {
    visit(fun);
    *ostream << std::endl;
  }
  *ostream << "assignments:" << std::endl;
  for (auto &assign : unit.assignments) {
    visit(assign);
    *ostream << std::endl;
  }
  *ostream << "module calls:" << std::endl;
  for (auto &call : unit.moduleCalls) {
    call->visit(*this);
    *ostream << std::endl;
  }
}

inline std::ostream &operator<<(std::ostream &os, UnaryOp op) {
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

inline std::ostream &operator<<(std::ostream &os, BinOp op) {
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
}  // namespace sscad
