class Test
{
public:
    template <int N, int ... Rest>
    int sum(){
      if constexpr(sizeof...(Rest) > 0) {
        return N + sum<Rest...>();
      } else {
        return N;
      }
    }


    template<typename I, typename ... Rest>
    int summ(I&& i, Rest&&... rest)
    {
        if constexpr(sizeof...(rest) > 0) {
            return i + summ(rest...);
        } else {
            return i;
        }
    }

    template<typename T> 
    class my_array {};
 
    // two type template parameters and one template template parameter:
    template<typename K, typename V, template<typename> typename C = my_array>
    class Map
    {
        C<K> key;
        C<V> value;
    };    
};


int main(){
  Test t;

    t.summ(1,2,3);

    Test::Map<int, int> a;

  return t.sum<1, 5, 7>();
}
