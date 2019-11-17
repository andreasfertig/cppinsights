#include <cstddef>
#include <tuple>
#include <vector>

struct Iterable {
  using Widget = std::tuple<int, float>;
  template <typename access_type>
  struct Iterator {
    Iterable*   m_iterable;
    std::size_t m_pos;
    bool        operator==( const Iterator& other ) { return m_iterable == other.m_iterable && m_pos == other.m_pos; }
    bool        operator!=( const Iterator& other ) { return !( this->operator==( other ) ); }
    access_type operator*() {
      Widget tmp = m_iterable->m_storage[m_pos];
      return std::get<access_type>( tmp );
    }
    Iterator operator++() {
      m_pos++;
      return *this;
    }
  };
  template <typename access_type = int>
  Iterator<access_type> begin() {
    return {this, 0};
  }
  template <typename access_type = int>
  Iterator<access_type> end() {
    return {this, m_storage.size()};
  }
  std::vector<Widget> m_storage;
};

void test() {
  Iterable container;
  for ( auto element : container ) { static_assert( std::is_same_v<decltype( element ), int> ); }
  for ( auto iter = container.begin<float>(); iter != container.end<float>(); ++iter ) {
    static_assert( std::is_same_v<decltype( *iter ), float> );
  }
}
