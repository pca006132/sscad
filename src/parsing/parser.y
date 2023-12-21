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
%require "3.2"
%language "c++"
%define api.parser.class { Parser }
%define api.namespace { sscad }
%define api.value.type variant
%define api.value.automove true
%define api.token.constructor
%define api.location.type { sscad::Location }
%define parse.trace
%define parse.assert true
%define parse.error detailed
%define parse.lac full
%lex-param { sscad::Scanner &scanner }
%parse-param { sscad::Scanner &scanner }
%parse-param { sscad::TranslationUnit &unit}

%code requires {
#include "ast.h"

namespace sscad {
  class Scanner;
  struct TranslationUnit;
}
}

%code top{
#include "frontend.h"
#include "parsing/scanner.h"

using namespace sscad;
#define YYMAXDEPTH 20000
#define yylex(scanner) scanner.getNextToken()
std::shared_ptr<ModuleCall> makeModifier(
   std::string modifier,
   std::shared_ptr<ModuleCall> child,
   Location loc);
static std::ostream& operator<<(std::ostream& o, const Location::Position& pos);
static std::ostream& operator<<(std::ostream& o, const Location& loc);
}

%token MODULE FUNCTION IF ELSE FOR LET EACH
%token TRUE FALSE UNDEF
%token END 0 "EOF"
%token NOT COMMA ASSIGN LPAREN RPAREN COLON SEMI QUESTION
%token LSQUARE RSQUARE LBRACE RBRACE HASH DOT
%token <std::string> ID
%token <std::string> STRING
%token <double> NUMBER
%token <BinOp> AND OR EQ NEQ GT GE LT LE ADD SUB MUL DIV MOD EXP

%nonassoc NO_ELSE
%nonassoc ELSE

%left OR
%left AND
%left EQ NEQ
%left GT GE LT LE
%left ADD SUB
%left MUL DIV MOD
%left EXP

%type <ModuleBody> child_statement child_statements
%type <std::shared_ptr<IfModule>> if_statement ifelse_statement
%type <std::shared_ptr<SingleModuleCall>> single_module_instantiation 
%type <std::shared_ptr<ModuleCall>> module_instantiation
%type <BinOp> binop
%type <Expr> expr call unary primary exponent
%type <std::string> module_id
%type <std::vector<AssignNode>> argument_list parameter_list
%type <AssignNode> argument parameter assignment

%%

input   : /* empty */
        | input statement
        ;

statement
        : SEMI
        | LBRACE inner_input RBRACE
        | module_instantiation
          { auto m = $1;
            if (m != nullptr)
              unit.moduleCalls.push_back(m); }
        | MODULE ID LPAREN RPAREN child_statement
          { unit.modules.push_back(ModuleDecl($2,
              std::vector<AssignNode>(), $5, @$)); }
        | MODULE ID LPAREN parameter_list optional_comma RPAREN child_statement
          { unit.modules.push_back(ModuleDecl($2, $4, $7, @$)); }
        ;

inner_input
        : /* empty */
        | inner_input statement
        ;

  /* note that only module_instantiation can give nullptr... */
module_instantiation
        : NOT module_instantiation
          { $$ = makeModifier("!", $2, @$); }
        | HASH module_instantiation
          { $$ = makeModifier("#", $2, @$); }
        | MOD module_instantiation
          { $$ = makeModifier("%", $2, @$); }
        | MUL module_instantiation
          /* remove this module */
          { $$ = nullptr; }
        | single_module_instantiation child_statement
          { auto mod = $1;
            mod->body = $2;
            $$ = mod; }
        | ifelse_statement
          { $$ = $1; }
        ;

ifelse_statement
        : if_statement ELSE child_statement
          { $$ = $1;
            $$->ifelse = $3; }
        | if_statement %prec NO_ELSE
          { $$ = $1; }
        ;

if_statement
        : IF LPAREN expr RPAREN child_statement
          { $$ = std::make_shared<IfModule>($3, $5, ModuleBody(), @$); }
        ;

assignment
        : ID EQ expr SEMI
          { $$ = AssignNode($1, $3, @$); }
        ;

child_statements
        : /* empty */
          { $$ = ModuleBody(); }
        | child_statements child_statement
          { $$ = $1;
            auto body = $2;
            $$.assignments.insert($$.assignments.end(),
                body.assignments.begin(), body.assignments.end());
            $$.children.insert($$.children.end(),
                body.children.begin(), body.children.end()); }
        | child_statements assignment
          { $$ = $1;
            $$.assignments.push_back($2); }
        ;

child_statement
        : SEMI
          { $$ = ModuleBody(); }
        | LBRACE child_statements RBRACE
          { $$ = $2; }
        | module_instantiation
          { auto m = $1;
            if (m == nullptr) $$ = ModuleBody();
            else $$ = ModuleBody(std::vector<AssignNode>(), {m}); }
        ;

single_module_instantiation
        : module_id LPAREN RPAREN
        { $$ = std::make_shared<SingleModuleCall>(
            $1, std::vector<AssignNode>(), ModuleBody(), @$); }
        | module_id LPAREN argument_list optional_comma RPAREN
        { $$ = std::make_shared<SingleModuleCall>($1, $3, ModuleBody(), @$); }
        ;

expr    : unary
        | expr binop unary { $$ = std::make_shared<BinaryOpNode>($1, $3, $2, @$);}
        ;

unary   : exponent
        | ADD unary { $$ = $2; }
        | SUB unary { $$ = std::make_shared<UnaryOpNode>($2, UnaryOp::NEG, @$); }
        | NOT unary { $$ = std::make_shared<UnaryOpNode>($2, UnaryOp::NOT, @$); }
        ;

exponent: call
        | call EXP unary { $$ = std::make_shared<BinaryOpNode>($1, $3, BinOp::EXP, @$); }
        ;

call    : primary
        | call LPAREN RPAREN
          { $$ = std::make_shared<CallNode>($1, std::vector<AssignNode>(), @$); }
        | call LPAREN argument_list optional_comma RPAREN
          { $$ = std::make_shared<CallNode>($1, $3, @$); }
          /* list lookup not implementd yet */
        ;

primary : TRUE               { $$ = std::make_shared<NumberNode>(1, @$); }
        | FALSE              { $$ = std::make_shared<NumberNode>(0, @$); }
        | NUMBER             { $$ = std::make_shared<NumberNode>($1, @$); }
        | STRING             { $$ = std::make_shared<StringNode>($1, @$); }
        | UNDEF              { $$ = std::make_shared<UndefNode>(@$); }
        | ID                 { $$ = std::make_shared<IdentNode>($1, @$); }
        | LPAREN expr RPAREN { $$ = $2; }
          /* lists are not handled for now */
        ;

parameter_list : parameter                      { $$ = {$1}; }
               | parameter_list COMMA parameter { $$ = $1; $$.push_back($3); }
               ;

parameter      : ID                             { $$ = AssignNode($1, nullptr, @$); }
               | ID ASSIGN expr                 { $$ = AssignNode($1, $3, @$); }
               ;

argument_list  : argument                       { $$ = {$1}; }
               | argument_list COMMA argument   { $$ = $1; $$.push_back($3); }
               ;

argument       : expr                           { $$ = AssignNode("", $1, @$); }
               | ID ASSIGN expr                 { $$ = AssignNode($1, $3, @$); }
               ;

module_id      : ID                             { $$ = $1; }
               | FOR                            { $$ = "for"; }
               | LET                            { $$ = "let"; }
               | EACH                           { $$ = "each"; }
               ;

binop   : AND | OR | EQ | NEQ | GT | GE | LT | LE | ADD | SUB | MUL | DIV | MOD;

optional_comma : /* empty */ | COMMA ;

%%

std::shared_ptr<ModuleCall> makeModifier(
   std::string modifier,
   std::shared_ptr<ModuleCall> child,
   Location loc
) {
  if (child == nullptr)
    return nullptr;
  return std::make_shared<ModuleModifier>(modifier, child, loc);
}

void sscad::Parser::error(const Location &loc, const std::string &message) {
  throw sscad::Parser::syntax_error(loc, message);
}

static std::ostream& operator<<(std::ostream& o,
                                const Location::Position& pos) {
  return o << pos.src << ":" << pos.line << ":" << pos.column;
}

static std::ostream& operator<<(std::ostream& o, const Location& loc) {
  return o << loc.begin << " - " << loc.end;
}

