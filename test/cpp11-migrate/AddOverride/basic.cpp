// RUN: grep -Ev "// *[A-Z-]+:" %s > %t.cpp
// RUN: cpp11-migrate -add-override %t.cpp -- -I %S -std=c++11
// RUN: FileCheck -input-file=%t.cpp %s

class A {
public:
  virtual ~A();
  // CHECK: virtual ~A();
  void f();
  virtual void h() const;
  // CHECK: virtual void h() const;
  virtual void i() = 0;
  // CHECK: virtual void i() = 0;
};

// Test that override isn't added to non-virtual functions.
class B : public A {
public:
  void f();
  // CHECK: class B
  // CHECK: void f();
};

// Test that override is added to functions that override virtual functions.
class C : public A {
public:
  void h() const;
  // CHECK: class C
  // CHECK: void h() const override;
};

// Test that override isn't add to functions that overload but not override.
class D : public A {
public:
  void h();
  // CHECK: class D
  // CHECK: void h();
};

// Test that override isn't added again to functions that already have it.
class E : public A {
public:
  void h() const override;
  // CHECK: class E
  // CHECK: void h() const override;
};

// Test that override isn't added to the destructor.
class F : public A {
public:
  virtual ~F();
  // CHECK: class F
  // CHECK: virtual ~F();
};

// Test that override is placed before any end of line comments.
class G : public A {
public:
  void h() const; // comment
  void i() // comment
  {}
  // CHECK: class G
  // CHECK: void h() const override; // comment
  // CHECK: void i() override // comment
  // CHECK-NEXT: {}
};

// Test that override is placed correctly if there is an inline body.
class H : public A {
public:
  void h() const { }
  // CHECK: class H
  // CHECK: void h() const override { }
};

// Test that override is placed correctly if there is a body on the next line.
class I : public A {
public:
  void h() const
  { }
  // CHECK: class I
  // CHECK: void h() const override
  // CHECK: { }
};

// Test that override is placed correctly if there is a body outside the class.
class J : public A {
public:
  void h() const;
  // CHECK: class J
  // CHECK: void h() const override;
};

void J::h() const {
  // CHECK: void J::h() const {
}

// Test that override is placed correctly if there is a trailing return type.
class K : public A {
public:
  auto h() const -> void;
  // CHECK: class K
  // CHECK: auto h() const -> void override;
};

// Test that override isn't added if it is already specified via a macro.
class L : public A {
public:
#define LLVM_OVERRIDE override
  void h() const LLVM_OVERRIDE;
  // CHECK: class L
  // CHECK: void h() const LLVM_OVERRIDE;
};

// Test that override isn't added at the wrong place for "pure overrides"
struct APure {
  virtual APure *clone() = 0;
};
struct BPure : APure {
  virtual BPure *clone() { return new BPure(); }
};
struct CPure : BPure {
  virtual BPure *clone() = 0;
  // CHECK: struct CPure : BPure {
  // CHECK-NOT: virtual BPure *clone() = 0 override;
  // CHECK: };
};
struct DPure : CPure {
  virtual DPure *clone() { return new DPure(); }
};

// Test that override is not added on dangerous template constructs
struct Base1 {
  virtual void f();
};
struct Base2 {};
template<typename T> struct Derived : T {
  void f(); // adding 'override' here will break instantiation of Derived<Base2>
  // CHECK: struct Derived
  // CHECK: void f();
};
Derived<Base1> d1;
Derived<Base2> d2;

template <typename T>
class M : public A {
public:
  virtual void i();
  // CHECK: class M : public A {
  // CHECK: virtual void i() override;
};
M<int> b;
