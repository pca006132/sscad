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
#include "driver.h"

namespace sscad {

Driver::Driver(FileResolver resolver, FileProvider provider)
    : scanner(*this),
      parser(scanner, *this),
      resolver(resolver),
      provider(provider) {}

int Driver::parse(FileHandle file) {
  auto stream = provider(file);
  assert(stream != nullptr);
  location = Location{
      {nullptr, file, 1, 1},
      {nullptr, file, 1, 1},
  };
  scanner.switch_streams(stream.get());
  istreams.push(std::move(stream));
  return parser.parse();
}

void Driver::addUse(const std::string &filename) {
  FileHandle file = resolver(filename, location.begin.src);
  uses.insert(file);
}

void Driver::lexerInclude(const std::string &filename) {
  FileHandle file = resolver(filename, location.begin.src);
  // avoid cyclic include by walking the include stack
  Location *loc = &location;
  while (loc) {
    if (file == loc->begin.src)
      throw Parser::syntax_error(location, "recursive include detected");
    loc = loc->begin.parent.get();
  }
  const auto parent = std::make_shared<Location>(location);
  auto stream = provider(file);
  assert(stream != nullptr);
  scanner.switch_streams(stream.get());
  istreams.push(std::move(stream));
  location = Location{
      {parent, file, 1, 1},
      {parent, file, 1, 1},
  };
}

// return true if the file is truely ended, false otherwise (include stack)
bool Driver::lexerFileEnd() {
  auto oldStream = std::move(istreams.top());
  istreams.pop();
  if (istreams.empty()) return true;
  assert(location.begin.parent != nullptr);
  location = *location.begin.parent;
  scanner.switch_streams(istreams.top().get());
  return false;
}
}  // namespace sscad
