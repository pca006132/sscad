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
#include <functional>
#include <stack>
#include <string>

#include "scanner.h"

namespace sscad {
class Driver {
 public:
  // first parameter: filename to be included/used
  // second parameter: source file handle for the include/use statement
  using FileResolver = std::function<FileHandle(std::string, FileHandle)>;
  // provide the input stream given a file handle
  using FileProvider = std::function<std::unique_ptr<std::istream>(FileHandle)>;

  Driver(FileResolver resolver, FileProvider provider);

  int parse(FileHandle file);
  friend class Parser;
  friend class Scanner;

 private:
  Scanner scanner;
  Parser parser;
  Location location;
  FileProvider provider;
  FileResolver resolver;
  std::stack<std::unique_ptr<std::istream>> istreams;

  void lexerInclude(std::string);
  bool lexerFileEnd();
};
}  // namespace sscad
