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
  std::vector<FunctionDecl> functions;
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
