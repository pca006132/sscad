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
#include <unordered_map>
#include <unordered_set>

#include "ast.h"

namespace sscad {
class Scanner;

/**
 * A translation unit, usually a single file but can include other files.
 * File scope variables are actually translation unit scoped.
 */
struct TranslationUnit {
  TranslationUnit(FileHandle file) : file(file) {}

  std::unordered_set<FileHandle> uses;
  std::vector<ModuleDecl> modules;
  std::vector<AssignNode> assignments;
  std::vector<std::shared_ptr<ModuleCall>> moduleCalls;
  FileHandle file;
};

/**
 * Parser frontend that handles use etc.
 */
class Frontend {
 public:
  // first parameter: filename to be included/used
  // second parameter: source file handle for the include/use statement
  using FileResolver = std::function<FileHandle(std::string, FileHandle)>;
  // provide the input stream given a file handle
  using FileProvider = std::function<std::unique_ptr<std::istream>(FileHandle)>;

  Frontend(FileResolver resolver, FileProvider provider);

  TranslationUnit& parse(FileHandle file);
  std::unordered_map<FileHandle, TranslationUnit> units;

  friend Scanner;

 private:
  FileProvider provider;
  FileResolver resolver;
};
}  // namespace sscad
