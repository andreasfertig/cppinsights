// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <cstdio>
#include <iostream>
#include <coroutine>
struct P {
    std::suspend_always initial_suspend()
    {
        return {};
    }
    void return_void() const noexcept
    {
      std::cout<<"return_void()\n";
    }
    std::coroutine_handle<> get_return_object()
    {
        return std::coroutine_handle<P>::from_promise(*this);
    };
    void unhandled_exception() { throw; }
    std::suspend_never final_suspend() noexcept
    {
        return {};
    }
};
struct R {
    R( std::coroutine_handle<> d) noexcept
        : data(d)
    {
    }
    std::coroutine_handle<> data;
    using promise_type = P;
};
R funcA(){
  std::cout<<"funcA_1\n";
  co_return;
  std::cout<<"funcA_2\n";
}
int main()
{
  funcA().data.resume();
  return	0;
}
