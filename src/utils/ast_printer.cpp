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
    visit(assign.expr);
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
    visit(assign.expr);
  }
  for (const auto &child : body.children) {
    if (!start) *ostream << ",";
    start = false;
    visit(child);
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
    visit(assign.expr);
  }
  *ostream << "), ";
  if (!single.body.children.empty() || !single.body.assignments.empty()) {
    *ostream << "body=";
    visit(single.body);
    *ostream << ", ";
  }
  *ostream << "loc=" << single.loc << ")";
}

void AstPrinter::visit(IfModule &ifmodule) {
  *ostream << "If(cond=";
  visit(ifmodule.args[0].expr);
  *ostream << ", then=";
  visit(ifmodule.ifthen);
  if (!ifmodule.ifelse.children.empty()) {
    *ostream << ", else=";
    visit(ifmodule.ifelse);
  }

  *ostream << ", loc=" << ifmodule.loc << ")";
}

void AstPrinter::visit(ModuleModifier &modifier) {
  *ostream << modifier.modifier;
  visit(modifier.module);
}

void AstPrinter::visit(ModuleDecl &module) {
  *ostream << "Module(" << module.name << ", args=(";
  bool start = true;
  for (const auto &assign : module.args) {
    if (!start) *ostream << ",";
    start = false;
    *ostream << assign.ident << "=";
    if (assign.expr != nullptr) visit(assign.expr);
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
    if (assign.expr != nullptr) visit(assign.expr);
  }
  *ostream << "), ";
  visit(fun.body);
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
  visit(unary.operand);
  *ostream << ", loc=" << unary.loc << ")";
}

void AstPrinter::visit(BinaryOpNode &binary) {
  *ostream << "Binary(" << binary.op << ", ";
  visit(binary.lhs);
  *ostream << ", ";
  visit(binary.rhs);
  *ostream << ", loc=" << binary.loc << ")";
}

void AstPrinter::visit(CallNode &call) {
  *ostream << "Call(";
  visit(call.fun);
  *ostream << ", args=(";
  bool start = true;
  for (const auto &assign : call.args) {
    if (!start) *ostream << ",";
    start = false;
    if (!assign.ident.empty()) *ostream << assign.ident << "=";
    visit(assign.expr);
  }
  *ostream << "), loc=" << call.loc << ")";
}

void AstPrinter::visit(IfExprNode &ifcond) {
  *ostream << "IfExpr(cond=";
  visit(ifcond.cond);
  *ostream << ", then=";
  visit(ifcond.ifthen);
  if (ifcond.ifelse != nullptr) {
    *ostream << ", else=";
    visit(ifcond.ifelse);
  }

  *ostream << ", loc=" << ifcond.loc << ")";
}

void AstPrinter::visit(ListExprNode &list) {
  *ostream << "List(";
  for (const auto &elem : list.elements) {
    if (elem.second) *ostream << "each ";
    visit(elem.first);
    *ostream << ", ";
  }
  *ostream << "loc=" << list.loc << ")";
}

void AstPrinter::visit(RangeNode &range) {
  *ostream << "range(";
  visit(range.start);
  *ostream << ", ";
  visit(range.step);
  *ostream << ", ";
  visit(range.end);
  *ostream << ", loc=" << range.loc << ")";
}

void AstPrinter::visit(ListCompNode &node) {
  *ostream << "listcomp(iters=(";
  for (const auto &iter : node.assignments) {
    *ostream << iter.ident << "=";
    visit(iter.expr);
    *ostream << ", ";
  }
  *ostream << "), generators=(";
  for (const auto &generator : node.generators) {
    *ostream << "(cond=";
    visit(std::get<0>(generator));
    *ostream << ", body=";
    if (std::get<2>(generator)) *ostream << "each ";
    visit(std::get<1>(generator));
    *ostream << "), ";
  }
  *ostream << "), loc=" << node.loc << ")";
}

void AstPrinter::visit(ListCompCNode &node) {
  *ostream << "listcompc(init=(";
  for (const auto &iter : node.init) {
    *ostream << iter.ident << "=";
    visit(iter.expr);
    *ostream << ", ";
  }
  *ostream << "), cond=";
  visit(node.cond);
  *ostream << "), update=(";
  for (const auto &iter : node.update) {
    *ostream << iter.ident << "=";
    visit(iter.expr);
    *ostream << ", ";
  }
  *ostream << "), generators=(";
  for (const auto &generator : node.generators) {
    *ostream << "(cond=";
    visit(std::get<0>(generator));
    *ostream << ", body=";
    if (std::get<2>(generator)) *ostream << "each ";
    visit(std::get<1>(generator));
    *ostream << "), ";
  }
  *ostream << "), loc=" << node.loc << ")";
}

void AstPrinter::visit(ListIndexNode &node) {
  *ostream << "index(";
  visit(node.list);
  *ostream << ", ";
  visit(node.index);
  *ostream << ", loc=" << node.loc << ")";
}

void AstPrinter::visit(LetNode &node) {
  *ostream << "let(bindings=(";
  for (const auto &iter : node.bindings) {
    *ostream << iter.ident << "=";
    visit(iter.expr);
    *ostream << ", ";
  }
  *ostream << "), ";
  visit(node.expr);
  *ostream << "), loc=" << node.loc << ")";
}

void AstPrinter::visit(LambdaNode &node) {
  *ostream << "lambda(bindings=(";
  for (const auto &iter : node.params) {
    *ostream << iter.ident << "=";
    if (iter.expr != nullptr) visit(iter.expr);
    *ostream << ", ";
  }
  *ostream << "), ";
  visit(node.expr);
  *ostream << "), loc=" << node.loc << ")";
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
    visit(call);
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
