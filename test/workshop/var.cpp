// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: clang-rename foo bar %t.cpp --
// RUN: FileCheck %s < %t.cpp

// CHECK: int bar;
int foo;

// CHECK: int i = bar;
int i = foo;

// CHECK: void f(char *bar = 0) {
void f(char *foo = 0) {
  // CHECK: f(bar);
  f(foo);

  {
    // CHECK: int bar;
    int foo;
  }
}
