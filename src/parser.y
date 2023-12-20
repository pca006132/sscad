/**
 * Copyright (C) 2023 The sscad Authors.
 * Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                         Marius Kintel <marius@kintel.net>
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
 *
 * =====================================================================
 *
 * Note that some code is copied from openscad parser/lexer
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
%lex-param { sscad::Driver &driver }
%parse-param { sscad::Scanner &scanner }
%parse-param { sscad::Driver &driver }

%code requires {
#include "ast.h"

namespace sscad {
  class Scanner;
  class Driver;
}
}

%code top{
#include <iostream>
#include <stack>
#include "driver.h"

using namespace sscad;
#define YYMAXDEPTH 20000
#define yylex(scanner, driver) scanner.getNextToken()
std::shared_ptr<ModuleCall> makeModifier(
   std::string modifier,
   std::shared_ptr<ModuleCall> child,
   Location loc);
}

%token MODULE FUNCTION IF ELSE FOR LET EACH
%token TRUE FALSE UNDEF
%token END 0 "EOF"
%token NOT COMMA ASSIGN LPAREN RPAREN SEMI
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

%type <std::shared_ptr<IfModule>> if_statement ifelse_statement
%type <std::shared_ptr<SingleModuleCall>> single_module_instantiation 
%type <std::shared_ptr<ModuleCall>> module_instantiation
%type <BinOp> binop
%type <Expr> expr call unary primary exponent expr_or_empty
%type <std::string> module_id
%type <std::vector<AssignNode>> argument_list parameter_list
%type <AssignNode> argument parameter

%%

input
        : /* empty */
        | input statement
        ;

statement
        : SEMI
        | LBRACE inner_input RBRACE
        | module_instantiation
          { driver.moduleCalls.push_back($1); }
        | MODULE ID LPAREN RPAREN statement
          { driver.modules.push_back(ModuleDecl($2,
              std::vector<AssignNode>(), driver.getBody(), @$)); }
        | MODULE ID LPAREN parameter_list optional_comma RPAREN statement
          { driver.modules.push_back(ModuleDecl($2, $4, driver.getBody(), @$)); }
        | END
        ;

inner_input
        : /* empty */
        | inner_input statement
        ;

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
            mod->body = driver.getBody();
            $$ = mod; }
        | ifelse_statement
          { $$ = $1; }
        ;

ifelse_statement
        : if_statement %prec NO_ELSE
          { $$ = $1; }
        | if_statement ELSE child_statement
          { $$ = $1;
            $$->ifelse = driver.getBody(); }
        ;

if_statement
        : IF LPAREN expr RPAREN child_statement
          { auto body = driver.getBody();
            $$ = std::make_shared<IfModule>($3, body, ModuleBody(), @$); }
        ;

assignment
        : ID EQ expr SEMI
          { driver.assignments.push_back(AssignNode($1, $3, @$)); }
        ;

child_statements
        : /* empty */
        | child_statements child_statement
        | child_statements assignment
        ;

child_statement
        : SEMI
        | LBRACE child_statements RBRACE
        | module_instantiation
          { driver.moduleCalls.push_back($1); }
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

exponent : call
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

expr_or_empty  : /* empty */ { $$ = nullptr; }
               | expr { $$ = $1; }
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
