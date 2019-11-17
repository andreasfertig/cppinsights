#include <cstddef>

struct Iterable {
  template <typename access_type>
  struct Iterator {
    Iterable*   m_iterable;
    int dummy;
    bool        operator!=( const Iterator& other ) { return m_iterable != other.m_iterable; }
    access_type operator*() {
      return dummy;
    }

    Iterator operator++() {
      return *this;
    }
  };

  template <typename access_type = int>
  Iterator<access_type> begin() {
    return {this, 2};
  }

  template <typename access_type = int>
  Iterator<access_type> end() {
    return {this, 2};
  }
};

void test() {
  Iterable container;
  for ( auto element : container ) {  }
  for ( auto iter = container.begin<float>(); iter != container.end<float>(); ++iter ) { }
}

