# Copyright 2023 The sscad Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_executable(parserTest parser_test.cpp)
target_link_libraries(parserTest sscad)
target_compile_features(parserTest PUBLIC cxx_std_17)

add_executable(evalTest evaluator_test.cpp)
target_link_libraries(evalTest sscad)
target_compile_features(evalTest PUBLIC cxx_std_17)
