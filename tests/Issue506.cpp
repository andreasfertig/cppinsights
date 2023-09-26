#include <cstdio>
#include <iostream>
#include <cctype>

template<class...Match>
bool search(char ch, Match&&...matchers) {
   return (matchers(ch) || ...);
}

int main()
{
  std::cout << search('A', ::isalpha, ::isdigit, [](char ch) { return ch == '_'; });
}

