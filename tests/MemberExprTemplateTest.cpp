  class X {
      public:
    template<typename T> void f(T);

    template<typename... Args> void ff(Args...);
  };


  void test(X x) {
    [&]() {
        x.f<int>(17);
        x.ff<int>(17, 2, 55.5);
    }();

}
