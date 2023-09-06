template <typename T>
struct foo {
  struct bar;
};

template <typename T>
struct foo<T>::bar {};

int main() {
  foo<int>::bar b;
}
