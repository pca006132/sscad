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
#include <memory>
#include <ostream>
#include <string>

namespace sscad {
using FileHandle = unsigned long;

struct Location {
  struct Position {
    // parent is the parent file that includes the current file
    // there can be a chain of parents
    std::shared_ptr<Location> parent = nullptr;
    FileHandle src = 0;
    long line = 1;
    long column = 1;
  };

  Position begin;
  Position end;

  void reset() { end = begin = {nullptr, 0, 1, 1}; }
  void step() { begin = end; }
  void columns(long count = 1) { end.column += count; }
  void lines(long count = 1) {
    if (count) end.column = 0;
    end.line += count;
  }
  void lines(const std::string& text) {
    for (size_t i = 0; i < text.length(); ++i) {
      if (text[i] == '\r' && i + 1 < text.length() && text[i + 1] == '\n') {
        end.line++;
        i += 1;
      } else if (text[i] == '\r' || text[i] == '\n') {
        end.line++;
      }
    }
  }
};

inline std::ostream& operator<<(std::ostream& o,
                                const Location::Position& pos) {
  return o << pos.src << ":" << pos.line << ":" << pos.column;
}
inline std::ostream& operator<<(std::ostream& o, const Location& loc) {
  return o << loc.begin << " - " << loc.end;
}
}  // namespace sscad
