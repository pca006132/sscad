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

#ifndef yyFlexLexerOnce
#undef yyFlexLexer
#define yyFlexLexer sscad_FlexLexer
#include <FlexLexer.h>
#endif

#include "parser.h"
#undef YY_DECL
#define YY_DECL sscad::Parser::symbol_type sscad::Scanner::getNextToken()

using namespace std::string_literals;

namespace U_ICU_NAMESPACE {
class BreakIterator;
}

namespace sscad {
class Driver;

class Scanner : public yyFlexLexer {
 public:
  Scanner(Driver &driver);
  virtual ~Scanner();
  virtual sscad::Parser::symbol_type getNextToken();

 private:
  Driver &driver;
  std::string stringcontents;
  U_ICU_NAMESPACE::BreakIterator *brkiter;

  // negative if the string is not a valid identifier
  int numGraphemes(const char *str);
  Parser::symbol_type parseNumber(const std::string &str,
                                  const Location loc) const;
  std::string toUTF8(int c) const;
};

}  // namespace sscad
