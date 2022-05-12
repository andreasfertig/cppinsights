// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#if __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace std {
    using namespace std::experimental;
}
#elif __has_include(<coroutine>)
#include <coroutine>
#else
#error "No coroutine header"
#endif

struct generator {
  struct promise_type {
    int current_value{};

    std::suspend_always yield_value(int value) {
      current_value = value;
      return {};
    }
    std::suspend_always initial_suspend() { return {}; }
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


generator seq(const int& start) {
  struct S { int t; char c; };
      
  const S s{}; // this will fail to compile in the transformation

  co_return s.t;
}

int main() {
  auto s = seq(3);

}
