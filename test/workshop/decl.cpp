// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: clang-rename foo bar %t.cpp --
// RUN: FileCheck %s < %t.cpp

class foo {};

// CHECK: bar f;
foo f;
