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

#include <iostream>
#include <sstream>

#include "codegen/const_eval.h"
#include "codegen/bytecode_gen.h"
#include "frontend.h"
#include "utils/ast_printer.h"

using namespace sscad;
using namespace std::string_literals;

int main() {
  auto printer = AstPrinter(&std::cout);
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
        (*ss) << "echo(a + b(123, c = 456));\n"
                 "function foo(x) = x + 1;";
        break;
      case 1:
        (*ss) << "include<a>\n"
                 "echo(foo + naïve);\n"
                 "foo2(123) { cube(); }\n"
                 "module foo2(a, b = 2) { cube(); children(); }\n"
                 "*if (1+1==2) cube();\n"
                 "if (1+1==2) { a(foo() ? x : y + 2); } else { b(); }";
        break;
      case 2:
        (*ss) << "a = 1;\n"
                 "b = 2;\n"
                 "a = b + 1;\n"
                 "function foo(a, b) = a > 0 ? foo(a-1, b+2) : b;\n"
                 "echo(-(1 + 1 == 2 ? 5 : 6));";
        break;
      case 3:
        (*ss) << "echo(a * b + c * d > 12 && foo ^ bar);\r\n"
                 "echo(a+b+c\n+d);\nfoo@";
        break;
    }
    return ss;
  };

  Frontend fe(resolver, provider);
  ConstEvaluator const_eval;
  BytecodeGen generator;
  AstVisitor* const_eval_ptr = &const_eval;
  try {
    printer.visit(fe.parse(0));
    std::cout << "===================" << std::endl;
    printer.visit(fe.parse(1));
    std::cout << "===================" << std::endl;
    auto module = fe.parse(2);
    const_eval_ptr->visit(module);
    generator.visit(module);
    printer.visit(module);
    std::cout << "===================" << std::endl;
    printer.visit(fe.parse(3));
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}
