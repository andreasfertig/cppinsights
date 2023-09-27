template<unsigned int TA>
struct SA
{
  SA (const int & PA);

 ~SA();

  bool Test();
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

template<unsigned int TA>
SA<TA>::~SA()
{
}


template<unsigned int TA>
inline bool SA<TA>::Test()
{
    return false;
}


int main()
{
    int d;
    SA<2> iso(d);
}
