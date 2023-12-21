# Copyright (C) 2023 The sscad Authors.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
find_package(FLEX REQUIRED)
find_package(ICU COMPONENTS uc REQUIRED)

BISON_TARGET(Parser src/parsing/parser.y
             ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
             DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)

FLEX_TARGET(Scanner src/parsing/scanner.l
            ${CMAKE_CURRENT_BINARY_DIR}/scanner.cpp)

ADD_FLEX_BISON_DEPENDENCY(Scanner Parser)

add_library(sscad src/parsing/frontend.cpp src/parsing/scanner_helper.cpp
    ${BISON_Parser_OUTPUTS} ${FLEX_Scanner_OUTPUTS})
target_include_directories(sscad PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(sscad PUBLIC src)
target_link_libraries(sscad PRIVATE ICU::uc)
target_compile_features(sscad PRIVATE cxx_std_17)

add_subdirectory(test)
