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
