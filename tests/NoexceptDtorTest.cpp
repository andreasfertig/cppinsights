struct A {
	~A() {}
};

struct C { 
  int i; 

  // remains unevaluated as int doesn't require a dtor  
  ~C() = default;
};


struct D { 
  A a{};
  
  // unevaluated as not instantiated
  ~D() = default;
};


struct E { 
  A a{};
  
  // implicitly noexcept
  ~E() = default;
};

int main()
{ 
  C c{};
  E e{}; 
}

