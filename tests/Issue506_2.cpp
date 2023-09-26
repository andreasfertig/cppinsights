bool Fun(char)
{
    return false;
}

template<class...Match>
bool search(char ch, Match&&...matchers) {
   return (matchers(ch) || ...);
}

int main()
{
//  std::cout << search('A', ::isalpha, ::isdigit, [](char ch) { return ch == '_'; });
//  return search('A', Fun, Fun, [](char ch) { return ch == '_'; });
  return search('A', Fun);
}
