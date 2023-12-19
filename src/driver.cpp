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
  scanner.switch_streams(&*stream);
  istreams.push(std::move(stream));
  return parser.parse();
}

void Driver::lexerInclude(std::string filename) {
  FileHandle file = resolver(filename, location.begin.src);
  // avoid cyclic include by walking the include stack
  Location *loc = &location;
  while (loc) {
    if (file == loc->begin.src)
      throw Parser::syntax_error(location, "recursive include detected");
    loc = &*loc->begin.parent;
  }
  const auto parent = std::make_shared<Location>(location);
  auto stream = provider(file);
  assert(stream != nullptr);
  scanner.switch_streams(&*stream);
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
  scanner.switch_streams(&*istreams.top());
  return false;
}
}  // namespace sscad
