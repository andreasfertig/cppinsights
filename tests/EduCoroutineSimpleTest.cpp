// cmdline:-std=c++2a
// cmdlineinsights:-edu-show-coroutine-transformation


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

#include <new>
#include <utility>

struct generator {
  struct promise_type {
    int current_value{};

    std::suspend_always yield_value(int value) {
      current_value = value;
      return {};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    generator get_return_object() { return generator{this}; };
    void unhandled_exception() { std::terminate(); }
    void return_value(int v) { current_value = v; }
  };

  generator(generator &&rhs) : p{std::exchange(rhs.p, nullptr)} {}
  ~generator() { if (p) { p.destroy(); } }

private:
  explicit generator(promise_type* _p)
      : p{std::coroutine_handle<promise_type>::from_promise(*_p)} {}

  std::coroutine_handle<promise_type> p;
};

generator fun() {
  co_return 2;
}

int main() {
  auto s = fun();
}

