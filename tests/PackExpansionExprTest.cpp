void f(int,int,int) {
}

template<typename F, typename ...Types>
void forward(F f, Types &&...args) {
  f(static_cast<Types&&>(args)...);
}


int main()
{
    forward(f,1, 2,3);    
}

