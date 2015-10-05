// RUN: %python %S/check_clang_tidy.py %s modernize-loop-convert %t -- -std=c++11 -I %S/Inputs/modernize-loop-convert

#include "structures.h"

namespace Array {

const int N = 6;
const int NMinusOne = N - 1;
int Arr[N] = {1, 2, 3, 4, 5, 6};
const int ConstArr[N] = {1, 2, 3, 4, 5, 6};
int (*PArr)[N] = &Arr;

void f() {
  int Sum = 0;

  for (int I = 0; I < N; ++I) {
    Sum += Arr[I];
    int K;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead [modernize-loop-convert]
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: Sum += Elem;
  // CHECK-FIXES-NEXT: int K;

  for (int I = 0; I < N; ++I) {
    printf("Fibonacci number is %d\n", Arr[I]);
    Sum += Arr[I] + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  for (int I = 0; I < N; ++I) {
    int X = Arr[I];
    int Y = Arr[I] + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: int X = Elem;
  // CHECK-FIXES-NEXT: int Y = Elem + 2;

  for (int I = 0; I < N; ++I) {
    int X = N;
    X = Arr[I];
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: int X = N;
  // CHECK-FIXES-NEXT: X = Elem;

  for (int I = 0; I < N; ++I) {
    Arr[I] += 1;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: Elem += 1;

  for (int I = 0; I < N; ++I) {
    int X = Arr[I] + 2;
    Arr[I]++;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: int X = Elem + 2;
  // CHECK-FIXES-NEXT: Elem++;

  for (int I = 0; I < N; ++I) {
    Arr[I] = 4 + Arr[I];
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: Elem = 4 + Elem;

  for (int I = 0; I < NMinusOne + 1; ++I) {
    Sum += Arr[I];
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: Sum += Elem;

  for (int I = 0; I < N; ++I) {
    printf("Fibonacci number %d has address %p\n", Arr[I], &Arr[I]);
    Sum += Arr[I] + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Arr)
  // CHECK-FIXES-NEXT: printf("Fibonacci number %d has address %p\n", Elem, &Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  Val Teas[N];
  for (int I = 0; I < N; ++I) {
    Teas[I].g();
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Tea : Teas)
  // CHECK-FIXES-NEXT: Tea.g();
}

const int *constArray() {
  for (int I = 0; I < N; ++I) {
    printf("2 * %d = %d\n", ConstArr[I], ConstArr[I] + ConstArr[I]);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : ConstArr)
  // CHECK-FIXES-NEXT: printf("2 * %d = %d\n", Elem, Elem + Elem);

  const NonTriviallyCopyable NonCopy[N]{};
  for (int I = 0; I < N; ++I) {
    printf("2 * %d = %d\n", NonCopy[I].X, NonCopy[I].X + NonCopy[I].X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : NonCopy)
  // CHECK-FIXES-NEXT: printf("2 * %d = %d\n", Elem.X, Elem.X + Elem.X);

  bool Something = false;
  for (int I = 0; I < N; ++I) {
    if (Something)
      return &ConstArr[I];
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : ConstArr)
  // CHECK-FIXES-NEXT: if (Something)
  // CHECK-FIXES-NEXT: return &Elem;
}

struct HasArr {
  int Arr[N];
  Val ValArr[N];
  void implicitThis() {
    for (int I = 0; I < N; ++I) {
      printf("%d", Arr[I]);
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : Arr)
    // CHECK-FIXES-NEXT: printf("%d", Elem);

    for (int I = 0; I < N; ++I) {
      printf("%d", ValArr[I].X);
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : ValArr)
    // CHECK-FIXES-NEXT: printf("%d", Elem.X);
  }

  void explicitThis() {
    for (int I = 0; I < N; ++I) {
      printf("%d", this->Arr[I]);
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : this->Arr)
    // CHECK-FIXES-NEXT: printf("%d", Elem);

    for (int I = 0; I < N; ++I) {
      printf("%d", this->ValArr[I].X);
    }
    // CHECK-MESSAGES: :[[@LINE-3]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : this->ValArr)
    // CHECK-FIXES-NEXT: printf("%d", Elem.X);
  }
};

// Loops whose bounds are value-dependent should not be converted.
template <int N>
void dependentExprBound() {
  for (int I = 0; I < N; ++I)
    Arr[I] = 0;
}
template void dependentExprBound<20>();

void memberFunctionPointer() {
  Val V;
  void (Val::*mfpArr[N])(void) = {&Val::g};
  for (int I = 0; I < N; ++I)
    (V.*mfpArr[I])();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : mfpArr)
  // CHECK-FIXES-NEXT: (V.*Elem)();

  struct Foo {
    int (Val::*f)();
  } Foo[N];

  for (int I = 0; I < N; ++I)
    int R = (V.*(Foo[I].f))();
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Foo)
  // CHECK-FIXES-NEXT: int R = (V.*(Elem.f))();

}

} // namespace Array

namespace Iterator {

void f() {
  /// begin()/end() - based for loops here:
  T Tt;
  for (T::iterator It = Tt.begin(), E = Tt.end(); It != E; ++It) {
    printf("I found %d\n", *It);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Tt)
  // CHECK-FIXES-NEXT: printf("I found %d\n", Elem);

  T *Pt;
  for (T::iterator It = Pt->begin(), E = Pt->end(); It != E; ++It) {
    printf("I found %d\n", *It);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : *Pt)
  // CHECK-FIXES-NEXT: printf("I found %d\n", Elem);

  S Ss;
  for (S::iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Ss)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  S *Ps;
  for (S::iterator It = Ps->begin(), E = Ps->end(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & P : *Ps)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", P.X);

  for (S::const_iterator It = Ss.cbegin(), E = Ss.cend(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : Ss)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  for (S::iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    printf("s has value %d\n", It->X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Ss)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  for (S::iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    It->X = 3;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Ss)
  // CHECK-FIXES-NEXT: Elem.X = 3;

  for (S::iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    (*It).X = 3;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Ss)
  // CHECK-FIXES-NEXT: Elem.X = 3;

  for (S::iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    It->nonConstFun(4, 5);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Ss)
  // CHECK-FIXES-NEXT: Elem.nonConstFun(4, 5);

  U Uu;
  for (U::iterator It = Uu.begin(), E = Uu.end(); It != E; ++It) {
    printf("s has value %d\n", It->X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Uu)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  for (U::iterator It = Uu.begin(), E = Uu.end(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Uu)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  U::iterator A;
  for (U::iterator I = Uu.begin(), E = Uu.end(); I != E; ++I)
    int K = A->X + I->X;
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Uu)
  // CHECK-FIXES-NEXT: int K = A->X + Elem.X;

  dependent<int> V;
  for (dependent<int>::iterator It = V.begin(), E = V.end();
       It != E; ++It) {
    printf("Fibonacci number is %d\n", *It);
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : V)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);

  for (dependent<int>::iterator It(V.begin()), E = V.end();
       It != E; ++It) {
    printf("Fibonacci number is %d\n", *It);
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : V)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);

  doublyDependent<int, int> Intmap;
  for (doublyDependent<int, int>::iterator It = Intmap.begin(), E = Intmap.end();
       It != E; ++It) {
    printf("Intmap[%d] = %d", It->first, It->second);
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Intmap)
  // CHECK-FIXES: printf("Intmap[%d] = %d", Elem.first, Elem.second);

  // PtrSet's iterator dereferences by value so auto & can't be used.
  {
    PtrSet<int *> Val_int_ptrs;
    for (PtrSet<int *>::iterator I = Val_int_ptrs.begin(),
                                 E = Val_int_ptrs.end();
         I != E; ++I) {
      (void) *I;
    }
    // CHECK-MESSAGES: :[[@LINE-5]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto Val_int_ptr : Val_int_ptrs)
  }

  // This container uses an iterator where the derefence type is a typedef of
  // a reference type. Make sure non-const auto & is still used. A failure here
  // means canonical types aren't being tested.
  {
    TypedefDerefContainer<int> Int_ptrs;
    for (TypedefDerefContainer<int>::iterator I = Int_ptrs.begin(),
                                              E = Int_ptrs.end();
         I != E; ++I) {
      (void) *I;
    }
    // CHECK-MESSAGES: :[[@LINE-5]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Int_ptr : Int_ptrs)
  }

  {
    // Iterators returning an rvalue reference should disqualify the loop from
    // transformation.
    RValueDerefContainer<int> Container;
    for (RValueDerefContainer<int>::iterator I = Container.begin(),
                                             E = Container.end();
         I != E; ++I) {
      (void) *I;
    }
  }

  dependent<Val *> Dpp;
  for (dependent<Val *>::iterator I = Dpp.begin(), E = Dpp.end(); I != E; ++I) {
    printf("%d\n", (**I).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Dpp)
  // CHECK-FIXES-NEXT: printf("%d\n", (*Elem).X);

  for (dependent<Val *>::iterator I = Dpp.begin(), E = Dpp.end(); I != E; ++I) {
    printf("%d\n", (*I)->X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : Dpp)
  // CHECK-FIXES-NEXT: printf("%d\n", Elem->X);
}

// Tests to verify the proper use of auto where the init variable type and the
// initializer type differ or are mostly the same except for const qualifiers.
void different_type() {
  // Ss.begin() returns a type 'iterator' which is just a non-const pointer and
  // differs from const_iterator only on the const qualification.
  S Ss;
  for (S::const_iterator It = Ss.begin(), E = Ss.end(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : Ss)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", Elem.X);

  S *Ps;
  for (S::const_iterator It = Ps->begin(), E = Ps->end(); It != E; ++It) {
    printf("s has value %d\n", (*It).X);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto P : *Ps)
  // CHECK-FIXES-NEXT: printf("s has value %d\n", P.X);

  // V.begin() returns a user-defined type 'iterator' which, since it's
  // different from const_iterator, disqualifies these loops from
  // transformation.
  dependent<int> V;
  for (dependent<int>::const_iterator It = V.begin(), E = V.end();
       It != E; ++It) {
    printf("Fibonacci number is %d\n", *It);
  }

  for (dependent<int>::const_iterator It(V.begin()), E = V.end();
       It != E; ++It) {
    printf("Fibonacci number is %d\n", *It);
  }
}

// Tests to ensure that an implicit 'this' is picked up as the container.
// If member calls are made to 'this' within the loop, the transform becomes
// risky as these calls may affect state that affects the loop.
class C {
public:
  typedef MutableVal *iterator;
  typedef const MutableVal *const_iterator;

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  void doSomething();
  void doSomething() const;

  void doLoop() {
    for (iterator I = begin(), E = end(); I != E; ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : *this)

    for (iterator I = C::begin(), E = C::end(); I != E; ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : *this)

    for (iterator I = begin(), E = end(); I != E; ++I) {
      (void) *I;
      doSomething();
    }

    for (iterator I = begin(); I != end(); ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : *this)

    for (iterator I = begin(); I != end(); ++I) {
      (void) *I;
      doSomething();
    }
  }

  void doLoop() const {
    for (const_iterator I = begin(), E = end(); I != E; ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto Elem : *this)

    for (const_iterator I = C::begin(), E = C::end(); I != E; ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto Elem : *this)

    for (const_iterator I = begin(), E = end(); I != E; ++I) {
      (void) *I;
      doSomething();
    }
  }
};

class C2 {
public:
  typedef MutableVal *iterator;

  iterator begin() const;
  iterator end() const;

  void doLoop() {
    // The implicit 'this' will have an Implicit cast to const C2* wrapped
    // around it. Make sure the replacement still happens.
    for (iterator I = begin(), E = end(); I != E; ++I)
      (void) *I;
    // CHECK-MESSAGES: :[[@LINE-2]]:5: warning: use range-based for loop instead
    // CHECK-FIXES: for (auto & Elem : *this)
  }
};

} // namespace Iterator

namespace PseudoArray {

const int N = 6;
dependent<int> V;
dependent<int> *Pv;
const dependent<NonTriviallyCopyable> Constv;
const dependent<NonTriviallyCopyable> *Pconstv;

transparent<dependent<int>> Cv;

void f() {
  int Sum = 0;
  for (int I = 0, E = V.size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", V[I]);
    Sum += V[I] + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : V)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  for (int I = 0, E = V.size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", V.at(I));
    Sum += V.at(I) + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : V)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  for (int I = 0, E = Pv->size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", Pv->at(I));
    Sum += Pv->at(I) + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : *Pv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  // This test will fail if size() isn't called repeatedly, since it
  // returns unsigned int, and 0 is deduced to be signed int.
  // FIXME: Insert the necessary explicit conversion, or write out the types
  // explicitly.
  for (int I = 0; I < Pv->size(); ++I) {
    printf("Fibonacci number is %d\n", (*Pv).at(I));
    Sum += (*Pv)[I] + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : *Pv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;

  for (int I = 0; I < Cv->size(); ++I) {
    printf("Fibonacci number is %d\n", Cv->at(I));
    Sum += Cv->at(I) + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : *Cv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem);
  // CHECK-FIXES-NEXT: Sum += Elem + 2;
}

// Ensure that 'const auto &' is used with containers of non-trivial types.
void constness() {
  int Sum = 0;
  for (int I = 0, E = Constv.size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", Constv[I].X);
    Sum += Constv[I].X + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : Constv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem.X);
  // CHECK-FIXES-NEXT: Sum += Elem.X + 2;

  for (int I = 0, E = Constv.size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", Constv.at(I).X);
    Sum += Constv.at(I).X + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : Constv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem.X);
  // CHECK-FIXES-NEXT: Sum += Elem.X + 2;

  for (int I = 0, E = Pconstv->size(); I < E; ++I) {
    printf("Fibonacci number is %d\n", Pconstv->at(I).X);
    Sum += Pconstv->at(I).X + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : *Pconstv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem.X);
  // CHECK-FIXES-NEXT: Sum += Elem.X + 2;

  // This test will fail if size() isn't called repeatedly, since it
  // returns unsigned int, and 0 is deduced to be signed int.
  // FIXME: Insert the necessary explicit conversion, or write out the types
  // explicitly.
  for (int I = 0; I < Pconstv->size(); ++I) {
    printf("Fibonacci number is %d\n", (*Pconstv).at(I).X);
    Sum += (*Pconstv)[I].X + 2;
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (const auto & Elem : *Pconstv)
  // CHECK-FIXES-NEXT: printf("Fibonacci number is %d\n", Elem.X);
  // CHECK-FIXES-NEXT: Sum += Elem.X + 2;
}

void constRef(const dependent<int>& ConstVRef) {
  int sum = 0;
  // FIXME: This does not work with size_t (probably due to the implementation
  // of dependent); make dependent work exactly like a std container type.
  for (int i = 0; i < ConstVRef.size(); ++i) {
    sum += ConstVRef[i];
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : ConstVRef)
  // CHECK-FIXES-NEXT: sum += Elem;

  for (auto I = ConstVRef.begin(), E = ConstVRef.end(); I != E; ++I) {
    sum += *I;
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : ConstVRef)
  // CHECK-FIXES-NEXT: sum += Elem;
}

// Check for loops that don't mention containers.
void noContainer() {
  for (auto I = 0; I < V.size(); ++I) {
  }

  for (auto I = 0; I < V.size(); ++I)
    ;
}

struct NoBeginEnd {
  unsigned size() const;
  unsigned& operator[](int);
  const unsigned& operator[](int) const;
};

struct NoConstBeginEnd {
  NoConstBeginEnd();
  unsigned size() const;
  unsigned* begin();
  unsigned* end();
  unsigned& operator[](int);
  const unsigned& operator[](int) const;
};

struct ConstBeginEnd {
  ConstBeginEnd();
  unsigned size() const;
  unsigned* begin() const;
  unsigned* end() const;
  unsigned& operator[](int);
  const unsigned& operator[](int) const;
};

// Shouldn't transform pseudo-array uses if the container doesn't provide
// begin() and end() of the right const-ness.
void NoBeginEndTest() {
  NoBeginEnd NBE;
  for (unsigned I = 0, E = NBE.size(); I < E; ++I)
    printf("%d\n", NBE[I]);

  const NoConstBeginEnd Const_NCBE;
  for (unsigned I = 0, E = Const_NCBE.size(); I < E; ++I)
    printf("%d\n", Const_NCBE[I]);

  ConstBeginEnd CBE;
  for (unsigned I = 0, E = CBE.size(); I < E; ++I)
    printf("%d\n", CBE[I]);
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto & Elem : CBE)
  // CHECK-FIXES-NEXT: printf("%d\n", Elem);

  const ConstBeginEnd Const_CBE;
  for (unsigned I = 0, E = Const_CBE.size(); I < E; ++I)
    printf("%d\n", Const_CBE[I]);
  // CHECK-MESSAGES: :[[@LINE-2]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : Const_CBE)
  // CHECK-FIXES-NEXT: printf("%d\n", Elem);
}

struct DerefByValue {
  DerefByValue();
  struct iter { unsigned operator*(); };
  unsigned size() const;
  iter begin();
  iter end();
  unsigned operator[](int);
};

void derefByValueTest() {
  DerefByValue DBV;
  for (unsigned I = 0, E = DBV.size(); I < E; ++I) {
    printf("%d\n", DBV[I]);
  }
  // CHECK-MESSAGES: :[[@LINE-3]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : DBV)
  // CHECK-FIXES-NEXT: printf("%d\n", Elem);

  for (unsigned I = 0, E = DBV.size(); I < E; ++I) {
    auto f = [DBV, I]() {};
    printf("%d\n", DBV[I]);
  }
  // CHECK-MESSAGES: :[[@LINE-4]]:3: warning: use range-based for loop instead
  // CHECK-FIXES: for (auto Elem : DBV)
  // CHECK-FIXES-NEXT: auto f = [DBV, &Elem]() {};
  // CHECK-FIXES-NEXT: printf("%d\n", Elem);
}

} // namespace PseudoArray
