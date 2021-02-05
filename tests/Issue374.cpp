struct S;

struct T
{
  int S::* s_mptr;
  int T::* t_mptr;
  static int T::* static_mptr;

  template<int S::*> void f();
  template<int T::*> void f();
  
  template<int (S::*)()> void f();
  template<int (T::*)()> void f();
  
  struct N
  {
    int S::* s_mptr;
    int T::* t_mptr;
    int N::* n_mptr;
  };
  int N::* n_mptr;
};

int T::* T::static_mptr = 0;
template<int T::*>     void T::f() {}
template<int (T::*)()> void T::f() {}
