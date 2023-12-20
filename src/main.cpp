#include "driver.h"

using namespace std::string_literals;

int main() {
  auto resolver = [](std::string name, sscad::FileHandle src) {
    if (name == "a") return 0;
    if (name == "b") return 1;
    if (name == "c") return 2;
    throw std::runtime_error("unknown file "s + name);
  };
  auto provider = [](sscad::FileHandle src) {
    auto ss = std::make_unique<std::stringstream>();
    switch (src) {
      case 0:
        (*ss) << "echo(a + b(123, c = 456));";
        break;
      case 1:
        (*ss) << "include<a>\n"
                 "echo(foo + naïve);\n"
                 "foo2(123) { cube(); }\n"
                 "module foo2(a, b = 2) { cube(); children(); }\n"
                 "if (1+1==2) cube();\n"
                 "if (1+1==2) { a(); } else { b(); }";
        break;
      case 2:
        (*ss) << "echo(a * b + c * d > 12 && foo ^ "
                 "bar);\r\necho(a+b+c\n+d);\nfoo?";
        break;
    }
    return ss;
  };

  sscad::Driver driver(resolver, provider);
  try {
    driver.parse(0);
    std::cout << "finished parsing 0" << std::endl;
    driver.parse(1);
    std::cout << "finished parsing 1" << std::endl;
    driver.parse(2);
    std::cout << "finished parsing 2" << std::endl;
  } catch (const sscad::Parser::syntax_error &e) {
    std::cerr << "Error at " << e.location << ":\n" << e.what() << std::endl;
  }
  return 0;
}
