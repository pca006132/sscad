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
#include "frontend.h"

#include "parsing/scanner.h"

namespace sscad {

Frontend::Frontend(FileResolver resolver, FileProvider provider)
    : resolver(resolver), provider(provider) {}

TranslationUnit& Frontend::parse(FileHandle file) {
  auto pair = units.insert({file, TranslationUnit(file)});
  if (!pair.second) return pair.first->second;

  auto& unit = pair.first->second;

  auto stream = provider(file);
  assert(stream != nullptr);

  Scanner scanner(*this, unit, std::move(stream));
  Parser parser(scanner, unit);
  parser.parse();

  for (const auto use : unit.uses) {
    parse(use);
  }
  return unit;
}

}  // namespace sscad
