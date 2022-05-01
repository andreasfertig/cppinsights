#include <cstdio>
#include <cstdlib>

void* operator new(size_t size)
{
  return std::malloc(size);
}

void operator delete(void* p) noexcept
{
  std::free(p);
}

class Apple {
public:
  // implicitly static
  void* operator new(size_t size)
  {
    return std::malloc(size);
  }

  void operator delete(void* p)
  {
    std::free(p);
  }
};

class Orange {
public:
  // only one static
  static void* operator new(size_t size)
  {
    return std::malloc(size);
  }

  static void operator delete(void* p)
  {
    std::free(p);
  }
};

int main()
{
}
