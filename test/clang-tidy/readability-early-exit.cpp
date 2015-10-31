// RUN: %check_clang_tidy %s readability-early-exit %t



void g();
void f1(bool a) {
  if (a) {
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: use early exit [readability-early-exit]
    g();
  }
}
// CHECK-FIXES: if (!(a)) return;
// CHECK-FIXES-NEXT: //
// CHECK-FIXES-NEXT: g();
// CHECK-FIXES: }
// CHECK-FIXES-NOT: }
// CHECK-FIXES: f2

void f2(bool a) {
  if (a) {
    g();
  }
  g();
}

void f3(bool a) {
  for (int i = 0; i < 3; ++i) {
    if (a) {
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: use early exit [readability-early-exit]
      g();
    }
  }
}
