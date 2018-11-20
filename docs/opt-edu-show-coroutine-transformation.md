# edu-show-coroutine-transformation {#edu_show_coroutine_transformation}
Show transformations of coroutines.

__Default:__ Off

__Examples:__

```.cpp
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace std {
    using namespace std::experimental;
}
#else
#error "No coroutine header"
#endif

#include <cstdio>
#include <exception>
#include <new>

struct generator {
    struct promise_type {
        int current_value{};

        std::suspend_always yield_value(int value)
        {
            current_value = value;
            return {};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        generator           get_return_object() { return generator{this}; };
        void                unhandled_exception() { std::terminate(); }
        void                return_value(int v) { current_value = v; }
    };

    generator(generator const&) = delete;
    generator(generator&& rhs)
    : p{std::exchange(rhs.p, nullptr)}
    {}

    ~generator()
    {
        if(handle) { handle.destroy(); }
    }

    void run()
    {
        if(not handle.done()) { handle.resume(); }
    }

    auto value() { return handle.promise().current_value; }

private:
    explicit generator(promise_type* p)
    : handle{std::coroutine_handle<promise_type>::from_promise(*p)}
    {}

    std::coroutine_handle<promise_type> handle;
};

generator fun()
{
    printf("Hello,");

    co_yield 4;

    printf("C++ Insights.\n");

    co_return 2;
}

int main()
{
    auto s = fun();

    s.run();

    printf("value: %d\n", s.value());

    s.run();

    printf("value: %d\n", s.value());
}
```

transforms into this:

```.cpp
/*************************************************************************************
 * NOTE: The coroutine transformation you've enabled is a hand coded transformation! *
 *       Most of it is _not_ present in the AST. What you see is an approximation.   *
 *************************************************************************************/
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace std {
    using namespace std::experimental;
}
#else
#error "No coroutine header"
#endif

#include <cstdio>
#include <exception>
#include <new>

struct generator
{
  struct promise_type
  {
    int current_value{};
    inline int yield_value(int value)
    {
      this->current_value = value;
      return {};
    }
    
    inline int initial_suspend()
    {
      return {};
    }
    
    inline int final_suspend() noexcept
    {
      return {};
    }
    
    inline generator get_return_object()
    {
      return generator{this};
    }
    
    inline void unhandled_exception()
    {
      std::terminate();
    }
    
    inline void return_value(int v)
    {
      this->current_value = v;
    }
    
  };
  
  // inline generator(const generator &) = delete;
  inline generator(generator && rhs)
  {
  }
  
  inline ~generator() noexcept
  {
    if() {
    } 
    
  }
  
  inline void run()
  {
    if() {
    } 
    
  }
  
  inline void value()
  {
  }
  
  
  private: 
  inline explicit generator(promise_type * p)
  {
  }
  
  int handle;
  public: 
};



generator fun()
{
  printf("Hello,");
  printf("C++ Insights.\n");
}


int main()
{
  generator s = fun();
  s.run();
  printf("value: %d\n", s.value());
  s.run();
  printf("value: %d\n", s.value());
  return 0;
}



```
