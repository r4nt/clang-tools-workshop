// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: clang-rename foo bar %t.cpp --
// RUN: FileCheck %s < %t.cpp

class foo {};

// CHECK: bar f;
foo f;

class zoo {};

// CHECK: zoo z;
zoo z;

namespace a {
class foo {};
}

// CHECK: a::bar g;
a::foo g;

// CHECK: C<const ::a::bar&> h;
template<typename T> class C {};
C<const ::a::foo&> h;

#define M(X) a::X**

// CHECK: void func(const M(bar) i);
void func(const M(foo) i);
