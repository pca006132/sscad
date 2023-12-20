# sscad

Simple scad interpreter. Work in progress, nothing works now.

This is intended as a library, not battery included.
Users of the library should provide:

1. File system access.
2. Geometry backend.
3. Model (de)serialization tools.

## Goals

1. Simpler code.

   The aim is to have a simpler implementation of the OpenSCAD's parser, lexer
   and evaluator. This should have a more straightforward logic and hopefully
   more concise code. Note that performance is more important than simplicity
   here, I will trade simplicity for performance given that it is not too
   complex.

2. Better abstractions.

   A parser should not perform file access, it should delegate the task to the
   consumer of the library. Likewise, the evaluator should not implement the
   actual geometry algorithm, a backend should provide that.

3. Better performance.

   Switching to a bytecode interpreter and designing with performance in mind
   should provide a better performance.


Explicit non-goal: 100% compatibility with OpenSCAD 2021.01. In particular, I
will happily break things that I think is weird and only a minority of users are
using, and happily give errors instead of warnings when things are easily
fixable (e.g. newlines in include and use).

## Design

### Parser and Scanner

It now uses Bison's C++ API, which makes the parser pure and enable the use of
smart pointers everywhere. Operator precedence is used instead of breaking the
grammar rules the old fashioned way. The parser and lexer are made as simple as
possible, computations are moved elsewhere. For example, we do not perform
optimizations like combining `-(123)` into a single AST node, instead we rely on
later optimization passes to do that.

We also support *unicode identifiers* now (by ID_START and ID_CONTINUE
properties), with proper support for newline characters and count unicode
*grapheme clusters* for column number.

### Location Tracking and File Access

Instead of only tracking the location in the current included file, we track the
stack of includes. Each file is represented by a file handle, two files are the
same file if they have the same exact handle, and cyclic include is forbidden.
An external provider will perform the file lookup (path to file handle) and
reading the file (file handle to istream).

### Evaluator

The evaluator is based on a simple stack-based bytecode format.
It provides a small set of instructions for stack manipulation, primitive
operations and function call.
Common patterns, such as adding a constant to a number, have their own
instructions to improve performance in the common case.

