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
#include "ast_visitor.h"
namespace sscad {
class AstPrinter : public AstVisitor {
 public:
  AstPrinter(std::ostream *ostream) : ostream(ostream) {}
  virtual void visit(AssignNode &) override;
  virtual void visit(ModuleBody &) override;
  virtual void visit(SingleModuleCall &) override;
  virtual void visit(IfModule &) override;
  virtual void visit(ModuleModifier &) override;
  virtual void visit(ModuleDecl &) override;
  virtual void visit(FunctionDecl &) override;
  virtual void visit(NumberNode &) override;
  virtual void visit(StringNode &) override;
  virtual void visit(UndefNode &) override;
  virtual void visit(IdentNode &) override;
  virtual void visit(UnaryOpNode &) override;
  virtual void visit(BinaryOpNode &) override;
  virtual void visit(CallNode &) override;
  virtual void visit(IfExprNode &) override;
  virtual void visit(ListExprNode &) override;
  virtual void visit(RangeNode &) override;
  virtual void visit(ListCompNode &) override;
  virtual void visit(ListCompCNode &) override;
  virtual void visit(ListIndexNode &) override;
  virtual void visit(LetNode &) override;
  virtual void visit(LambdaNode &) override;
  virtual void visit(TranslationUnit &unit) override;

  // TODO: add new AST nodes

  // https://isocpp.org/wiki/faq/strange-inheritance#hiding-rule
  using AstVisitor::visit;

 private:
  std::ostream *ostream;
};

std::ostream &operator<<(std::ostream &os, UnaryOp op);
std::ostream &operator<<(std::ostream &os, BinOp op);

}  // namespace sscad
