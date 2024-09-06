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
#include <memory>

#include "ast.h"
#include "frontend.h"

namespace sscad {
class AstVisitor {
 public:
  void visit(Node &);

  template <typename T, typename = std::enable_if_t<std::is_base_of_v<Node, T>>>
  void visit(std::shared_ptr<T> n) {
    visit(*n);
  }

  virtual void visit(AssignNode &);
  virtual void visit(SingleModuleCall &);
  virtual void visit(IfModule &);
  virtual void visit(ModuleModifier &);
  virtual void visit(ModuleDecl &);
  virtual void visit(FunctionDecl &);
  virtual void visit(NumberNode &){};
  virtual void visit(StringNode &){};
  virtual void visit(UndefNode &){};
  virtual void visit(IdentNode &){};
  virtual void visit(UnaryOpNode &);
  virtual void visit(BinaryOpNode &);
  virtual void visit(CallNode &);
  virtual void visit(IfExprNode &);
  virtual void visit(ListExprNode &);
  virtual void visit(RangeNode&);
  virtual void visit(ListCompNode&);
  virtual void visit(ListCompCNode&);
  virtual void visit(ListIndexNode&);
  virtual void visit(LetNode&);
  virtual void visit(LambdaNode&);
  virtual void visit(ModuleBody &);
  virtual void visit(TranslationUnit &);
};

class ExprMap : public AstVisitor {
 public:
  using AstVisitor::visit;
  std::shared_ptr<ExprNode> map(ExprNode &);
  template <typename T,
            typename = std::enable_if_t<std::is_base_of_v<ExprNode, T>>>
  std::shared_ptr<ExprNode> map(std::shared_ptr<T> n) {
    return map(*n);
  }
  virtual void visit(AssignNode &) override;
  virtual std::shared_ptr<ExprNode> map(NumberNode &);
  virtual std::shared_ptr<ExprNode> map(StringNode &);
  virtual std::shared_ptr<ExprNode> map(UndefNode &);
  virtual std::shared_ptr<ExprNode> map(IdentNode &);
  virtual std::shared_ptr<ExprNode> map(UnaryOpNode &);
  virtual std::shared_ptr<ExprNode> map(BinaryOpNode &);
  virtual std::shared_ptr<ExprNode> map(CallNode &);
  virtual std::shared_ptr<ExprNode> map(IfExprNode &);
  virtual std::shared_ptr<ExprNode> map(ListExprNode &);
  virtual std::shared_ptr<ExprNode> map(RangeNode&);
  virtual std::shared_ptr<ExprNode> map(ListCompNode&);
  virtual std::shared_ptr<ExprNode> map(ListCompCNode&);
  virtual std::shared_ptr<ExprNode> map(ListIndexNode&);
  virtual std::shared_ptr<ExprNode> map(LetNode&);
  virtual std::shared_ptr<ExprNode> map(LambdaNode&);
};
}  // namespace sscad
