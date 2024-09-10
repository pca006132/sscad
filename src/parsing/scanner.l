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
%option c++
%option yyclass="Scanner"
%option prefix="sscad_"
%option nodefault
%option noyywrap
%{
#include <cstdlib>
#include "frontend.h"
#include "parsing/scanner.h"

using namespace sscad;
using namespace std::string_literals;
// update column information
#define YY_USER_ACTION loc.columns(yyleng);
%}

%x cond_comment cond_lcomment cond_string cond_include cond_use

D [0-9]
E [Ee][+-]?{D}+
H [0-9a-fA-F]

U       [\x80-\xbf]
U2      [\xc2-\xdf]
U3      [\xe0-\xef]
U4      [\xf0-\xf4]
UNICODE {U2}{U}|{U3}{U}{U}|{U4}{U}{U}{U}
NL      \n|\r\n|\r

%%

%{
loc.step();
%}

  /* include file BEGIN */
include[ \t\r\n]*"<"    { BEGIN(cond_include); loc.lines(yytext); }
<cond_include>{
[^\t\r\n>]*             { stringcontents = yytext; loc.columns(abs(numGraphemes(yytext))); }
">"                     { BEGIN(INITIAL); lexerInclude(stringcontents); }
\t
[\n\r]                  { throw Parser::syntax_error(loc, "Newline in include statement"); }
<<EOF>>                 { throw Parser::syntax_error(loc, "Unterminated include statement"); }
}
  /* include file END */

  /* use file BEGIN */
use[ \t\r\n]*"<"        { BEGIN(cond_use); loc.lines(yytext); }
<cond_use>{
[^\t\r\n>]*             { stringcontents = yytext; loc.columns(abs(numGraphemes(yytext))); }
">"                     { BEGIN(INITIAL); addUse(stringcontents); loc.step(); }
\t
[\n\r]                  { throw Parser::syntax_error(loc, "Newline in use statement"); }
<<EOF>>                 { throw Parser::syntax_error(loc, "Unterminated use statement"); }
}
  /* use file END */

  /* quoted string BEGIN */
\"                      { BEGIN(cond_string); stringcontents.clear(); }
<cond_string>{
\\n                     { stringcontents += '\n';  }
\\t                     { stringcontents += '\t'; }
\\r                     { stringcontents += '\r'; }
\\\\                    { stringcontents += '\\'; }
\\\"                    { stringcontents += '"'; }
\\x[0-7]{H}             { unsigned long i = std::strtoul(yytext + 2, NULL, 16);
                          stringcontents += (i == 0 ? ' ' : (unsigned char)(i & 0xff)); }
\\u{H}{4}|\\U{H}{6}     { char32_t c = std::strtoul(yytext + 2, NULL, 16);
                          stringcontents += toUTF8(c); }
[^\\\r\n\"]             { stringcontents += yytext; }
{NL}                    { loc.lines(); }
{UNICODE}               { stringcontents += yytext;
                          int unicodeLength = abs(numGraphemes(yytext));
                          loc.columns(unicodeLength - yyleng); }
\"                      { BEGIN(INITIAL);
                          return Parser::make_STRING(stringcontents, loc); }
.                       { stringcontents += yytext; }
<<EOF>>                 { throw Parser::syntax_error(loc, "Unterminated string"); }
}
  /* quoted string END */

  /* single-line comment BEGIN */
\/\/                    { BEGIN(cond_lcomment); }
<cond_lcomment>{
{NL}                    { BEGIN(INITIAL); loc.lines(); loc.step(); }
{UNICODE}               { int unicodeLength = abs(numGraphemes(yytext));
                          loc.columns(unicodeLength); }
.
<<EOF>>                 { BEGIN(INITIAL);
                          if (lexerFileEnd())
                            return Parser::make_END(loc);
                          else
                            loc.step(); }
}
  /* single-line comment END */

  /* multi-lines comment BEGIN */
"/*"                    { BEGIN(cond_comment); }
<cond_comment>{
"*/"                    { BEGIN(INITIAL); loc.step(); }
{NL}                    { loc.lines(); }
{UNICODE}               { int unicodeLength = abs(numGraphemes(yytext));
                          loc.columns(unicodeLength); }
.
<<EOF>>                 { throw Parser::syntax_error(loc, "Unterminated comment"); }
}
  /* multi-lines comment END */

{D}+{E}? |
{D}*\.{D}+{E}? |
{D}+\.{D}*{E}?          return parseNumber(yytext, loc);

  /* keywords BEGIN */
"module"                return Parser::make_MODULE(loc);
"function"              return Parser::make_FUNCTION(loc);
"if"                    return Parser::make_IF(loc);
"else"                  return Parser::make_ELSE(loc);
"let"                   return Parser::make_LET(loc);
"for"                   return Parser::make_FOR(loc);
"each"                  return Parser::make_EACH(loc);
"echo"                  return Parser::make_ECHO(loc);
"assert"                return Parser::make_ASSERT(loc);
"true"                  return Parser::make_TRUE(loc);
"false"                 return Parser::make_FALSE(loc);
"undef"                 return Parser::make_UNDEF(loc);
  /* keywords END */
"$"?[a-zA-Z0-9_]+       return Parser::make_ID(yytext, loc);

  /* aux symbols BEGIN */
"+"                     return Parser::make_ADD(BinOp::ADD, loc);
"-"                     return Parser::make_SUB(BinOp::SUB, loc);
"*"                     return Parser::make_MUL(BinOp::MUL, loc);
"/"                     return Parser::make_DIV(BinOp::DIV, loc);
"%"                     return Parser::make_MOD(BinOp::MOD, loc);
"^"                     return Parser::make_EXP(BinOp::EXP, loc);
"<"                     return Parser::make_LT(BinOp::LT,  loc);
"<="                    return Parser::make_LE(BinOp::LE,  loc);
">="                    return Parser::make_GE(BinOp::GE,  loc);
">"                     return Parser::make_GT(BinOp::GT,  loc);
"=="                    return Parser::make_EQ(BinOp::EQ,  loc);
"!="                    return Parser::make_NEQ(BinOp::NEQ, loc);
"&&"                    return Parser::make_AND(BinOp::AND, loc);
"||"                    return Parser::make_OR(BinOp::OR,  loc);
"!"                     return Parser::make_NOT(loc);
","                     return Parser::make_COMMA(loc);
"="                     return Parser::make_ASSIGN(loc);
"("                     return Parser::make_LPAREN(loc);
")"                     return Parser::make_RPAREN(loc);
";"                     return Parser::make_SEMI(loc);
"["                     return Parser::make_LSQUARE(loc);
"]"                     return Parser::make_RSQUARE(loc);
"{"                     return Parser::make_LBRACE(loc);
"}"                     return Parser::make_RBRACE(loc);
"#"                     return Parser::make_HASH(loc);
"."                     return Parser::make_DOT(loc);
":"                     return Parser::make_COLON(loc);
"?"                     return Parser::make_QUESTION(loc);
  /* aux symbols END */

  /* supports no-break space U+00A0 (UTF-8 encoded: C2A0) as well as other spaces */
[\xc2\xa0\t ]           { loc.step(); }
{NL}                    { loc.lines(); loc.step(); }
<<EOF>>                 { if (lexerFileEnd()) return Parser::make_END(loc); }
  /* unicode identifier fallback, slower but robust */
"$"?([a-zA-Z0-9_]|{UNICODE})+ {
                          int length = numGraphemes(yytext);
                          // update location
                          loc.columns(abs(length) - yyleng);
                          // check if it is a valid identifier
                          if (length < 0)
                            throw Parser::syntax_error(loc, "Illegal identifier "s + yytext);
                          return Parser::make_ID(yytext, loc); }
.                       { throw Parser::syntax_error(loc, "Unexpected character "s + yytext); }

