// dummy to have a range-based for ready container link thing
template<class T>
class MyArrayWrapper {
   T* data;
   int size;

public:
   int* begin() { return size>0 ? &data[0] : nullptr; }
   int* end()   { return size>0 ? &data[size-1] : nullptr; }
};



void f() {
  auto lambda = [](auto container) {
    for(auto test : container) { }
  };

  MyArrayWrapper<int> vec{};
  lambda(vec);
}
