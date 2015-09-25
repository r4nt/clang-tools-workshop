// RUN: %python %S/check_clang_tidy.py %s misc-awesome-functions %t

void g(int* p, int length);

void f() {
  int a[5];
  g(a, 5);     // BAD
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: array to pointer [misc-awesome-functions]
  g(&a[0], 1); // OK
}
