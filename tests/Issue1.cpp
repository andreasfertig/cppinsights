template<typename T> T foo() {return T(42); } 
template<typename T> T fooGood() {return T{42}; } 

int a = foo<int>();
int b = fooGood<int>();
