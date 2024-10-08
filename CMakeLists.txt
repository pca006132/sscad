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

cmake_minimum_required(VERSION 3.18)
project(sscad LANGUAGES CXX)
enable_testing()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(CMAKE_EXPORT_COMPILE_COMMANDS AND NOT EMSCRIPTEN)
    # for nixos
    set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES
        ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()

find_package(BISON REQUIRED)
message("Bison version: ${BISON_VERSION}")
find_package(FLEX REQUIRED)
message("Flex version: ${FLEX_VERSION}")
find_package(ICU COMPONENTS uc REQUIRED)
message("ICU version: ${ICU_VERSION}")

BISON_TARGET(Parser src/parsing/parser.y
             ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)

FLEX_TARGET(Scanner src/parsing/scanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

add_library(sscad
    src/ast_visitor.cpp
    src/parsing/frontend.cpp
    src/parsing/scanner_helper.cpp
    src/vm/evaluator.cpp
    src/vm/instructions.cpp
    src/vm/values.cpp
    src/utils/ast_printer.cpp
    ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
target_include_directories(sscad PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(sscad PUBLIC src)
target_link_libraries(sscad PRIVATE ICU::uc)
target_compile_features(sscad PUBLIC cxx_std_17)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# only needed for the evaluator performance
set_source_files_properties(src/vm/evaluator.cpp PROPERTIES COMPILE_FLAGS "-mllvm -align-all-nofallthru-blocks=6")
endif()

add_subdirectory(test)
