/**
 * Copyright (C) 2023 The sscad Authors.
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
 */
#pragma once

#include <istream>
#include <stack>

#ifndef yyFlexLexerOnce
#undef yyFlexLexer
#define yyFlexLexer sscad_FlexLexer
#include <FlexLexer.h>
#endif

#include "parser.h"
#undef YY_DECL
#define YY_DECL sscad::Parser::symbol_type sscad::Scanner::getNextToken()

namespace U_ICU_NAMESPACE {
class BreakIterator;
}

namespace sscad {
struct TranslationUnit;
class Frontend;

class Scanner : public yyFlexLexer {
 public:
  Scanner(Frontend &frontend, TranslationUnit &unit,
          std::unique_ptr<std::istream> istream);
  virtual ~Scanner();
  virtual sscad::Parser::symbol_type getNextToken();

 private:
  Frontend &frontend;
  TranslationUnit &unit;

  std::string stringcontents;
  U_ICU_NAMESPACE::BreakIterator *brkiter;
  Location loc;
  std::stack<std::unique_ptr<std::istream>> istreams;

  // negative if the string is not a valid identifier
  int numGraphemes(const char *str);
  static Parser::symbol_type parseNumber(const std::string &str,
                                         const Location loc);
  static std::string toUTF8(int c);
  void addUse(const std::string &);
  void lexerInclude(const std::string &);
  bool lexerFileEnd();
};

}  // namespace sscad
