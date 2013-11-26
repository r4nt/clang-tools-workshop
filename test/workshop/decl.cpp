// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: clang-rename foo bar %t.cpp --
// RUN: FileCheck %s < %t.cpp

// CHECK: class bar {};
class foo {};

// CHECK: bar f;
foo f;

class foz {};

// CHECK: foz g;
foz g;

namespace a {
  // CHECK: class bar {};
  class foo {};
}

// CHECK: a::bar h;
a::foo h;

// CHECK: int bar;
int foo;

// CHECK: int i = bar;
int i = foo;

// CHECK: void func(int bar) {
void func(int foo) {
  // CHECK: int i = bar;
  int i = foo;
}
