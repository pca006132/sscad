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
}

%token MODULE FUNCTION IF ELSE FOR LET EACH ASSERT ECHO
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
%type <Expr> expr expr2 call binary unary primary exponent
%type <std::string> module_id
%type <std::vector<AssignNode>> argument_list parameter_list assign_list
%type <std::vector<std::pair<Expr, bool>>> element_list
%type <std::vector<std::tuple<Expr, Expr, bool>>> generators
%type <AssignNode> argument parameter assignment

%%

input   : /* empty */
        | input statement
        ;

statement
        : SEMI
        | LBRACE inner_input RBRACE
        | assignment
          { unit.assignments.push_back($1); }
        | module_instantiation
          { auto m = $1;
            if (m != nullptr)
              unit.moduleCalls.push_back(m); }
        | MODULE ID LPAREN RPAREN child_statement
          { unit.modules.emplace_back($2, std::vector<AssignNode>(), $5, @$); }
        | MODULE ID LPAREN parameter_list optional_comma RPAREN child_statement
          { unit.modules.emplace_back($2, $4, $7, @$); }
        | FUNCTION ID LPAREN RPAREN ASSIGN expr SEMI
          { unit.functions.emplace_back($2, std::vector<AssignNode>(), $6, @$); }
        | FUNCTION ID LPAREN parameter_list optional_comma RPAREN ASSIGN expr SEMI
          { unit.functions.emplace_back($2, $4, $8, @$); }
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
        : ID ASSIGN expr SEMI
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

expr    : binary QUESTION expr COLON expr
          { $$ = std::make_shared<IfExprNode>($1, $3, $5, @$); }
        | LET LPAREN assign_list RPAREN expr
          { $$ = std::make_shared<LetNode>($3, $5, @$); }
        | FUNCTION LPAREN parameter_list RPAREN expr
          { $$ = std::make_shared<LambdaNode>($3, $5, @$); }
        | expr2
          { $$ = $1; }
        ;

expr2   : binary
        | LSQUARE RSQUARE
          { $$ = std::make_shared<ListExprNode>(std::vector<std::pair<Expr, bool>>(), @$); }
        | LSQUARE element_list optional_comma RSQUARE
          { $$ = std::make_shared<ListExprNode>($2, @$); }
        | LSQUARE expr COLON expr RSQUARE
          { $$ = std::make_shared<RangeNode>(
              $2, std::make_shared<NumberNode>(1, @$), $4, @$); }
        | LSQUARE expr COLON expr COLON expr RSQUARE
          { $$ = std::make_shared<RangeNode>( $2, $4, $6, @$); }
        | expr2 LSQUARE expr RSQUARE
          { $$ = std::make_shared<ListIndexNode>($1, $3, @$); }
        | expr2 DOT ID
          { auto s = $3;
            auto id = s == "x" ? 0 : s == "y" ? 1 : s == "z" ? 2 : -1;
            if (id == -1) throw sscad::Parser::syntax_error(@$, "unexpected " + s);
            $$ = std::make_shared<ListIndexNode>($1, std::make_shared<NumberNode>(id, @$), @$); }
          /* ECHO and ASSERT requires list support,
             the plan is to make them special functions that take two arguments,
             one in the form of a list and one for return */
        ;

/* Instead of using rule levels to handle binary operator precedence, we use
 * bison's precedence feature for operators */
binary  : unary
        | binary binop unary { $$ = std::make_shared<BinaryOpNode>($1, $3, $2, @$);}
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

parameter      : module_id                      { $$ = AssignNode($1, nullptr, @$); }
               | module_id ASSIGN expr          { $$ = AssignNode($1, $3, @$); }
               ;

argument_list  : argument                       { $$ = {$1}; }
               | argument_list COMMA argument   { $$ = $1; $$.push_back($3); }
               ;

argument       : expr                           { $$ = AssignNode("", $1, @$); }
               | module_id ASSIGN expr          { $$ = AssignNode($1, $3, @$); }
               ;

element_list   : expr                           { $$ = {std::make_pair($1, false)}; }
               | EACH expr                      { $$ = {std::make_pair($2, true)}; }
               | element_list COMMA expr        { $$ = $1; $$.emplace_back($3, false); }
               | element_list COMMA EACH expr   { $$ = $1; $$.emplace_back($4, true); }
               | FOR LPAREN assign_list RPAREN generators
                 { $$ = { std::make_pair(std::make_shared<ListCompNode>($3, $5, @$), true) }; }
               | element_list COMMA FOR LPAREN assign_list RPAREN generators
                 { $$ = $1; $$.emplace_back( std::make_shared<ListCompNode>($5, $7, @$), true); }
               | FOR LPAREN assign_list SEMI expr SEMI assign_list RPAREN generators
                 { $$ = { std::make_pair(std::make_shared<ListCompCNode>($3, $5, $7, $9, @$), true) }; }
               | element_list COMMA FOR LPAREN assign_list SEMI expr SEMI assign_list RPAREN generators
                 { $$ = $1; $$.emplace_back(std::make_shared<ListCompCNode>($5, $7, $9, $11, @$), true) ; }
               ;

assign_list    : ID ASSIGN expr { $$ = {AssignNode($1, $3, @$)}; }
               | assign_list COMMA ID ASSIGN expr { $$ = $1; $$.push_back(AssignNode($3, $5, @$)); }
               ;

generators     : IF LPAREN expr RPAREN expr ELSE generators
                 { $$ = $7; $$.insert($$.begin(), std::make_tuple($3, $5, false)); }
               | IF LPAREN expr RPAREN EACH expr ELSE generators
                 { $$ = $8; $$.insert($$.begin(), std::make_tuple($3, $6, true)); }
               | IF LPAREN expr RPAREN expr
                 { $$ = std::vector<std::tuple<Expr, Expr, bool>>();
                   $$.insert($$.begin(), std::make_tuple($3, $5, false)); }
               | IF LPAREN expr RPAREN EACH expr
                 { $$ = std::vector<std::tuple<Expr, Expr, bool>>();
                   $$.insert($$.begin(), std::make_tuple($3, $6, true)); }
               | expr
                 { $$ = std::vector<std::tuple<Expr, Expr, bool>>();
                   $$.insert($$.begin(), std::make_tuple(
                     std::make_shared<NumberNode>(1, @$), $1, false)); }
               | EACH expr
                 { $$ = std::vector<std::tuple<Expr, Expr, bool>>();
                   $$.insert($$.begin(), std::make_tuple(
                     std::make_shared<NumberNode>(1, @$), $2, true)); }
               ;

module_id      : ID                             { $$ = $1; }
               | FOR                            { $$ = "for"; }
               | LET                            { $$ = "let"; }
               | EACH                           { $$ = "each"; }
               | ASSERT                         { $$ = "assert"; }
               | ECHO                           { $$ = "echo"; }
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
