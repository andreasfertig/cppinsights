// cmdlineinsights:-edu-show-lifetime

#include <utility>

// From: https://eel.is/c++draft/class.temporary#6.11
struct S { int mi; const std::pair<int,int>& mp; };

int main()
{
  S a { 1, {2,3} };
  S* p = new S{ 1, {2,3} }; 
}
