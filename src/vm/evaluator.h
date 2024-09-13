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
#include <atomic>
#include <ostream>

#include "values.h"

namespace sscad {
struct FunctionEntry {
  std::vector<unsigned char> instructions;
  int parameters;
  bool isModule;
};

class Evaluator {
 public:
  Evaluator(std::ostream *ostream, std::vector<FunctionEntry> functions,
            std::vector<ValueTag> globalTags, std::vector<SValue> globalValues)
      : ostream(ostream),
        functions(functions),
        globalTags(globalTags),
        globalValues(globalValues) {}

  ValuePair eval(int id);
  void stop() { flag.store(false, std::memory_order_relaxed); }

 private:
  std::ostream *ostream;
  std::vector<FunctionEntry> functions;
  std::vector<ValueTag> globalTags;
  std::vector<SValue> globalValues;
  std::atomic<bool> flag = true;
};
}  // namespace sscad
