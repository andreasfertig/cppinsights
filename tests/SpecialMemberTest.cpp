// https://abseil.io/tips/61
#include <memory>

class C {
 public:
  C() = default;  // misleading: C has a deleted default constructor
 private:
  const int i;  // const => must always be initialized.
};

class D {
 public:
  D() = default;  // unsurprising, but not explicit: D has a default constructor
 private:
  std::unique_ptr<int> p;  // std::unique_ptr has a default constructor
};

