// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: clang-tidy %t.cpp -checks='(google-explicit-constructor|llvm-namespace-comment)' -disable-checks='' -fix -- > %t.msg 2>&1
// RUN: FileCheck -input-file=%t.cpp %s
// RUN: FileCheck -input-file=%t.msg -check-prefix=CHECK-MESSAGES %s

namespace i {
}
// CHECK: } // namespace i
// CHECK-MESSAGES: note: FIX-IT applied suggested code changes

class A { A(int i); };
// CHECK: class A { explicit A(int i); };
// CHECK-MESSAGES: note: FIX-IT applied suggested code changes
// CHECK-MESSAGES: clang-tidy applied 2 of 2 suggested fixes.
