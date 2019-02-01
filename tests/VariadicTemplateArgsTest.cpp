#define INSIGHTS_USE_TEMPLATE

void f(int,int,int) {
}

template<typename F, typename ...Types>
void forward(F ff, Types &&...args) {
  ff(args...);
}


int main()
{
    forward(f,1, 2,3);    
}


