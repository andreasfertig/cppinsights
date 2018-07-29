#define INSIGHTS_USE_TEMPLATE

#include <utility>

class X {
public:
  ~X() {}
  
  X(std::initializer_list<int> x) {}

  X(int z, std::initializer_list<int> x) {}

  X(std::initializer_list<int> x, int z) {}
  
  X(void *x, int y) {}

  X& operator+=(const std::initializer_list<int>& x)
  {
     return *this;
  }
};

struct V {
  V(int x) {}
  V(std::initializer_list<int> x) {}

};

struct Y { int x, y; };

void takes_y(Y y);

void bar(std::initializer_list<int> x) {}


template<typename T>
void something(std::initializer_list<T>) {}

void foo() {
    X{1, 2, 3};
    X{nullptr, 0};

    X x{3, 4, 5};
    X b{1};

    X{1, {2, 3}};
    X c{1, {2, 3}};

    X{{1, 2}, 3};
    X d{{1, 2}, 3};
    
    b += {2 ,4};

    takes_y({1, 2});

    bar( {1, 2} );

    something( {1 , 2} );

    something( {1.0f , 2.0f} );

    V v{1};
    V vv{1, 2};
}

