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

  void step() { begin = end; }
  void columns(long count = 1) { end.column += count; }
  void lines(long count = 1) {
    if (count) end.column = 1;
    end.line += count;
  }
  void lines(const std::string &text) {
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

inline std::ostream &operator<<(std::ostream &os,
                                const Location::Position &pos) {
  return os << pos.src << ":" << pos.line << ":" << pos.column;
}

inline std::ostream &operator<<(std::ostream &os, const Location &loc) {
  return os << "(" << loc.begin << "," << loc.end << ")";
}
}  // namespace sscad
