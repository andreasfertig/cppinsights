// cmdlineinsights:-edu-show-lifetime

template<class T, class U>
struct pair {
  T first;
  U second;

  pair(T&& t, U&& u)
  : first{t}
  , second{u}
  {}

  pair(const T& t, const U& u)
  : first{t}
  , second{u}
  {}
};

// From: https://eel.is/c++draft/class.temporary#6.11
struct S {
  int                   mi;
  const pair<int, int>& failingp;
};

int main()
{
  S invalids{1, {2, 3}};  // #A
}

