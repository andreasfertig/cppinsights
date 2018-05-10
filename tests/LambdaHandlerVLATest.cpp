#if 0
int Test(int n) 
{
    char buffer[n];


    [=]() mutable {
        int x = buffer[0];
    }();
}

int main()
{
    Test(2);
}
#endif


template<unsigned int TA>
struct SA
{
  SA (const int & PA);
  int nt;
};

template<typename TB>
inline void test(TB aa)
{
}

template<unsigned int TA>
inline SA<TA>::SA(const int & PA)
{
  float e[nt];
  
  test([&e](int i, int j){ return e[i] < e[j]; });
}

int main()
{
    int d;
    SA<2> iso(d);
}
