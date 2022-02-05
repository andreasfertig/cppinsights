#include <complex>
using namespace std;

int main () {
  
  int q = 2;
  ++q;
  q--;
  q += 2.0;
  
  complex<int> com1{3,4};
  complex<int> com2(com1);
  
  com1 += com2;
  com1 = com1 + com2;;
  
  return 0;
}
