// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <exception> // std::terminate
#include <new>
#include <utility>

template <typename T> struct generator {
  struct promise_type {
    T current_value{};

    std::suspend_always yield_value(T value) {
      current_value = value;
      return {};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    generator get_return_object() { return generator{this}; };
    void unhandled_exception() { std::terminate(); }
    void return_value(T v) {  current_value = v; }

    // gives us getReturnStmtOnAllocFailure
    static generator get_return_object_on_allocation_failure(){
      throw std::bad_alloc();
    }
  };

  generator(generator &&rhs) : p{std::exchange(rhs.p, nullptr)} {}
  ~generator() { if (p) { p.destroy(); } }

private:
  explicit generator(promise_type* _p)
      : p{std::coroutine_handle<promise_type>::from_promise(*_p)} {}

  std::coroutine_handle<promise_type> p;
};

template <typename T>
generator<T> fun() {
  co_return 2;
}


int main() {
  auto s = fun<int>();
}

