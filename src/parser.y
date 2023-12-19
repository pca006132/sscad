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
}

%token TOK_MODULE TOK_FUNCTION TOK_IF TOK_ELSE TOK_FOR TOK_LET TOK_EACH
%token TOK_TRUE TOK_FALSE TOK_UNDEF
%token END 0 "EOF"
%token NOT COMMA ASSIGN LPAREN RPAREN SEMI
%token <std::string> TOK_ID
%token <std::string> TOK_STRING
%token <double> TOK_NUMBER
%token <BinOp> AND OR EQ NEQ GT GE LT LE ADD SUB MUL DIV MOD EXP

%left OR
%left AND
%left EQ NEQ
%left GT GE LT LE
%left ADD SUB
%left MUL DIV MOD
%left EXP

%type <BinOp> binop
%type <Expr> expr call unary primary exponent expr_or_empty
%type <std::string> module_id
%type <std::vector<AssignNode>> argument_list parameter_list
%type <AssignNode> assignment argument parameter

%%

input
        : /* empty */
        | input statement
        ;

statement
        : SEMI
        | expr SEMI
          {
            std::stringstream ss;
            $1->repr(ss);
            std::cout << ss.str() << std::endl;
          }
        | END
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

primary : TOK_TRUE           { $$ = std::make_shared<NumberNode>(1, @$); }
        | TOK_FALSE          { $$ = std::make_shared<NumberNode>(0, @$); }
        | TOK_NUMBER         { $$ = std::make_shared<NumberNode>($1, @$); }
        | TOK_STRING         { $$ = std::make_shared<StringNode>($1, @$); }
        | TOK_UNDEF          { $$ = std::make_shared<UndefNode>(@$); }
        | TOK_ID             { $$ = std::make_shared<IdentNode>($1, @$); }
        | LPAREN expr RPAREN { $$ = $2; }
          /* lists are not handled for now */
        ;

expr_or_empty  : /* empty */ { $$ = nullptr; }
               | expr { $$ = $1; }
               ;

parameter_list : parameter                      { $$ = {$1}; }
               | parameter_list COMMA parameter { $$ = $1; $$.push_back($3); }
               ;

parameter      : TOK_ID                         { $$ = AssignNode($1, nullptr, @$); }
               | TOK_ID ASSIGN expr             { $$ = AssignNode($1, $3, @$); }
               ;

argument_list  : argument                       { $$ = {$1}; }
               | argument_list COMMA argument   { $$ = $1; $$.push_back($3); }
               ;

argument       : expr                           { $$ = AssignNode("", $1, @$); }
               | TOK_ID ASSIGN expr             { $$ = AssignNode($1, $3, @$); }
               ;

module_id      : TOK_ID                         { $$ = $1; }
               | TOK_FOR                        { $$ = "for"; }
               | TOK_LET                        { $$ = "let"; }
               | TOK_EACH                       { $$ = "each"; }
               ;

binop   : AND | OR | EQ | NEQ | GT | GE | LT | LE | ADD | SUB | MUL | DIV | MOD;

optional_comma : /* empty */ | COMMA ;

%%

void sscad::Parser::error(const Location &loc, const std::string &message) {
  throw sscad::Parser::syntax_error(loc, message);
}
