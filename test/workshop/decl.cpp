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

#define M(x) x
// CHECK: M(bar) k;
M(foo) k;

// CHECK: #define N(y) bar y;
#define N(y) foo y;
N(m);

template<typename T> class Y {};
// CHECK: Y<bar> l;
Y<foo> l;

namespace a {
  // CHECK: class bar {};
  class foo {};
}

// CHECK: a::bar h;
a::foo h;

// CHECK: const int bar = 42;
const int foo = 42;

// CHECK: int i = bar;
int i = foo;

// CHECK: void func(int bar) {
void func(int foo) {
  // CHECK: int i = bar;
  int i = foo;
}

struct A {
  // CHECK: static int bar;
  static int foo;
};

// CHECK: int j = A::bar;
int j = A::foo;

class C {
// CHECK: void bar() {
void foo() {
  // CHECK: bar();
  foo();

  // CHECK: this->bar();
  this->foo();
}
};

template<int> class Z {};
// CHECK: Z<bar> z;
Z<foo> z;
