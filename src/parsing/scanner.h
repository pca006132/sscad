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
