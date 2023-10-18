struct A { operator int&(); };
 
int& ir = A();

