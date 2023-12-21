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
#include <unicode/brkiter.h>
#include <unicode/bytestream.h>
#include <unicode/utypes.h>

#include "scanner.h"

namespace sscad {
Scanner::Scanner(Driver &driver) : driver(driver) {
  UErrorCode status;
  brkiter = icu::BreakIterator::createCharacterInstance(
      icu::Locale::getDefault(), status);
}

Scanner::~Scanner() { delete brkiter; }

int Scanner::numGraphemes(const char *str) {
  auto s = icu::UnicodeString::fromUTF8(str);
  brkiter->setText(s);
  int length = 0;
  bool validIdent = true;
  int c;
  while ((c = brkiter->next()) != icu::BreakIterator::DONE) {
    if (validIdent) {
      if (length == 0)
        validIdent = u_hasBinaryProperty(s.char32At(c - 1), UCHAR_ID_START) ||
                     s.char32At(c - 1) == '_';
      else if (!u_hasBinaryProperty(s.char32At(c - 1), UCHAR_ID_CONTINUE))
        validIdent = false;
    }
    length++;
  }
  if (!validIdent) length = -length;
  return length;
}

Parser::symbol_type Scanner::parseNumber(const std::string &str,
                                         const Location loc) const {
  errno = 0;
  double value = std::strtod(str.c_str(), nullptr);
  if (errno == 0) return Parser::make_NUMBER(value, loc);
  throw Parser::syntax_error(loc, "Invalid number \""s + str + '"');
}

std::string Scanner::toUTF8(int c) const {
  auto s = icu::UnicodeString::fromUTF32(&c, 1);
  std::string str;
  s.toUTF8String(str);
  return str;
}

}  // namespace sscad
