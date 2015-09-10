// RUN: %python %S/check_clang_tidy.py %s misc-sizeof-container %t

namespace std {

typedef unsigned int size_t;

template <typename T>
struct basic_string {
  size_t size() const;
};

template <typename T>
basic_string<T> operator+(const basic_string<T> &, const T *);

typedef basic_string<char> string;

template <typename T>
struct vector {
  size_t size() const;
};

class fake_container1 {
  size_t size() const; // non-public
};

struct fake_container2 {
  size_t size(); // non-const
};

}

using std::size_t;

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define ARRAYSIZE2(a) \
  (((sizeof(a)) / (sizeof(*(a)))) / static_cast<size_t>(!((sizeof(a)) % (sizeof(*(a))))))

struct string {
  std::size_t size() const;
};

template<typename T>
void g(T t) {
  (void)sizeof(t);
}

void f() {
  string s1;
  std::string s2;
  std::vector<int> v;

  int a = 42 + sizeof(s1);
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: sizeof() doesn't return the size of the container; did you mean .size()? [misc-sizeof-container]
// CHECK-FIXES: int a = 42 + s1.size();
  a = 123 * sizeof(s2);
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: sizeof() doesn't return the size
// CHECK-FIXES: a = 123 * s2.size();
  a = 45 + sizeof(s2 + "asdf");
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: sizeof() doesn't return the size
// CHECK-FIXES: a = 45 + (s2 + "asdf").size();
  a = sizeof(v);
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: sizeof() doesn't return the size
// CHECK-FIXES: a = v.size();
  a = sizeof(std::vector<int>{});
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: sizeof() doesn't return the size
// CHECK-FIXES: a = std::vector<int>{}.size();

  a = sizeof(a);
  a = sizeof(int);
  a = sizeof(std::string);
  a = sizeof(std::vector<int>);

  g(s1);
  g(s2);
  g(v);

  std::fake_container1 f1;
  std::fake_container2 f2;

  a = sizeof(f1);
  a = sizeof(f2);


  std::string arr[3];
  a = ARRAYSIZE(arr);
  a = ARRAYSIZE2(arr);
  a = sizeof(arr) / sizeof(arr[0]);

  (void)a;
}
