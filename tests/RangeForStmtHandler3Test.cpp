// http://coliru.stacked-crooked.com/a/8cbbd3c00d92d0a3
// https://stackoverflow.com/questions/8164567/how-to-make-my-custom-type-to-work-with-range-based-for-loops
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <iostream>

    struct null_sentinal_t {
      template<class Rhs,
        std::enable_if_t<!std::is_same<Rhs, null_sentinal_t>{},int> =0
      >
      friend bool operator==(Rhs const& ptr, null_sentinal_t) {
        return !*ptr;
      }
      template<class Rhs,
        std::enable_if_t<!std::is_same<Rhs, null_sentinal_t>{},int> =0
      >
      friend bool operator!=(Rhs const& ptr, null_sentinal_t) {
        return !(ptr==null_sentinal_t{});
      }
      template<class Lhs,
        std::enable_if_t<!std::is_same<Lhs, null_sentinal_t>{},int> =0
      >
      friend bool operator==(null_sentinal_t, Lhs const& ptr) {
        return !*ptr;
      }
      template<class Lhs,
        std::enable_if_t<!std::is_same<Lhs, null_sentinal_t>{},int> =0
      >
      friend bool operator!=(null_sentinal_t, Lhs const& ptr) {
        return !(null_sentinal_t{}==ptr);
      }
      friend bool operator==(null_sentinal_t, null_sentinal_t) {
        return true;
      }
      friend bool operator!=(null_sentinal_t, null_sentinal_t) {
        return false;
      }
    };

template<class Char>
struct str_view {
    Char const* ptr = 0;
    Char const* begin() const { return ptr; }
    null_sentinal_t end() const { return {}; }
};
int main() {
    
    for( char c : str_view<char>{"hello world\n"} )
    {
            std::cout << c;
    }

#if 0        
        auto&& range = str_view<char>{"hello world\n"};
        using std::begin; using std::end;
        auto start = range.begin();
        auto finish = range.end();
        for( ; start != finish; ++start )
        {
            char c=*start;
            std::cout << c;
        }
#endif
}
