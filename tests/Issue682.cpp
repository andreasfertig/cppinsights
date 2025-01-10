// cmdlineinsights:-edu-show-lifetime

void Fun() {}

  static_assert([] { return true;
  }());


  int main()
  {
    Fun();
    return 0;
  }

