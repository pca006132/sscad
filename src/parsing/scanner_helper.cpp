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

#include "frontend.h"
#include "scanner.h"

using namespace std::string_literals;

namespace sscad {
Scanner::Scanner(Frontend &frontend, TranslationUnit &unit,
                 std::unique_ptr<std::istream> istream)
    : frontend(frontend), unit(unit) {
  UErrorCode status;
  brkiter = icu::BreakIterator::createCharacterInstance(
      icu::Locale::getDefault(), status);
  Location::Position pos{nullptr, unit.file, 1, 1};
  loc = {pos, pos};
  istreams.push(std::move(istream));
  switch_streams(istreams.top().get());
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
                                         const Location loc) {
  errno = 0;
  double value = std::strtod(str.c_str(), nullptr);
  if (errno == 0) return Parser::make_NUMBER(value, loc);
  throw Parser::syntax_error(loc, "Invalid number \""s + str + '"');
}

std::string Scanner::toUTF8(int c) {
  auto s = icu::UnicodeString::fromUTF32(&c, 1);
  std::string str;
  s.toUTF8String(str);
  return str;
}

void Scanner::addUse(const std::string &filename) {
  FileHandle file = frontend.resolver(filename, loc.begin.src);
  unit.uses.insert(file);
}

void Scanner::lexerInclude(const std::string &filename) {
  FileHandle file = frontend.resolver(filename, loc.begin.src);
  // avoid cyclic include by walking the include stack
  Location *locPtr = &loc;
  while (true) {
    if (file == locPtr->begin.src)
      throw Parser::syntax_error(loc, "recursive include detected");
    if (locPtr->begin.parent == nullptr)
      break;
    locPtr = locPtr->begin.parent.get();
  }
  const auto parent = std::make_shared<Location>(loc);
  auto stream = frontend.provider(file);
  assert(stream != nullptr);
  switch_streams(stream.get());
  istreams.push(std::move(stream));
  loc = Location{
      {parent, file, 1, 1},
      {parent, file, 1, 1},
  };
}

// return true if the file is truely ended, false otherwise (include stack)
bool Scanner::lexerFileEnd() {
  auto oldStream = std::move(istreams.top());
  istreams.pop();
  if (istreams.empty()) return true;
  assert(loc.begin.parent != nullptr);
  loc = *loc.begin.parent;
  switch_streams(istreams.top().get());
  return false;
}

}  // namespace sscad
