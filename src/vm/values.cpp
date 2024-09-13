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
#include "values.h"

namespace sscad {
bool ValuePair::operator==(ValuePair rhs) const {
  if (tag != rhs.tag) return false;
  switch (tag) {
    case ValueTag::STRING:
      return value.s == rhs.value.s;
    case ValueTag::VECTOR:
      if (value.vec->values->size() != rhs.value.vec->values->size())
        return false;
      for (size_t i = 0; i < value.vec->values->size(); i++) {
        if (value.vec->values->at(i) != rhs.value.vec->values->at(i))
          return false;
      }
      return true;
    case ValueTag::RANGE:
      return *value.range == *rhs.value.range;
    case ValueTag::NUMBER:
      return value.number == rhs.value.number;
    case ValueTag::BOOLEAN:
      return value.cond == rhs.value.cond;
    default:
      return true;
  }
}
}  // namespace sscad
