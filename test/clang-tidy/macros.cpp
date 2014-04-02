// RUN: clang-tidy -checks=google-explicit-constructor -disable-checks='' %s -- | FileCheck %s

#define Q(name) class name { name(int i); }

Q(A);
// CHECK: :[[@LINE-1]]:3: warning: Single-argument constructors must be explicit [google-explicit-constructor]
// CHECK: :3:30: note: expanded from macro 'Q'
