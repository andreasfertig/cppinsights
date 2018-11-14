void f(int,int,int) {
}

template<typename F, typename ...Types>
auto forward(F f, Types &&...args) {
  return sizeof...(args);
}


int main()
{
    forward(f,1, 2,3);    
}

