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

#include <codecvt>
#include <locale>
#ifndef yyFlexLexerOnce
#undef yyFlexLexer
#define yyFlexLexer sscad_FlexLexer
#include <FlexLexer.h>
#endif

#include "parser.h"
#undef YY_DECL
#define YY_DECL sscad::Parser::symbol_type sscad::Scanner::getNextToken()

using namespace std::string_literals;

namespace sscad {
class Driver;

class Scanner : public yyFlexLexer {
 public:
  Scanner(Driver &driver) : driver(driver) {}
  virtual ~Scanner() {}
  virtual sscad::Parser::symbol_type getNextToken();

 private:
  Driver &driver;
  std::string stringcontents;
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;

  Parser::symbol_type parseNumber(const std::string &str, const Location loc) {
    errno = 0;
    double value = std::strtod(str.c_str(), nullptr);
    if (errno == 0) return Parser::make_TOK_NUMBER(value, loc);
    throw Parser::syntax_error(loc, "Invalid number \""s + str + '"');
  }
};

}  // namespace sscad
