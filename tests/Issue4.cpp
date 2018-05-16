#include <cstdio>
//#include <vector>
//#include <iostream>
using namespace std;

int main()
{
  	static const char arr[] = { 1,2,3 };
	auto&& l=[ uuu = arr[1]]{
    	printf("a: %c\n", uuu);
    };
}
