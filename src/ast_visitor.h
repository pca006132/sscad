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

namespace sscad {
struct AssignNode;
struct ModuleBody;
struct SingleModuleCall;
struct IfModule;
struct ModuleModifier;
struct ModuleDecl;
struct FunctionDecl;
struct ExprNode;
struct NumberNode;
struct StringNode;
struct UndefNode;
struct IdentNode;
struct UnaryOpNode;
struct BinaryOpNode;
struct CallNode;
struct IfExprNode;
struct TranslationUnit;

class AstVisitor {
 public:
  virtual void visit(AssignNode &);
  virtual void visit(ModuleBody &);
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
  virtual void visit(TranslationUnit &unit);
};

class ExprMap : public AstVisitor {
 public:
  virtual void visit(AssignNode &) override;
  virtual std::shared_ptr<ExprNode> map(NumberNode &);
  virtual std::shared_ptr<ExprNode> map(StringNode &);
  virtual std::shared_ptr<ExprNode> map(UndefNode &);
  virtual std::shared_ptr<ExprNode> map(IdentNode &);
  virtual std::shared_ptr<ExprNode> map(UnaryOpNode &);
  virtual std::shared_ptr<ExprNode> map(BinaryOpNode &);
  virtual std::shared_ptr<ExprNode> map(CallNode &);
  virtual std::shared_ptr<ExprNode> map(IfExprNode &);
};
}  // namespace sscad
