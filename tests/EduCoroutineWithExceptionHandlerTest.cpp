// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <exception> // std::terminate
#include <new>
#include <utility>

struct suspend_never_bool {
    bool await_ready() const noexcept { return true; }
    bool await_suspend(std::coroutine_handle<void>) noexcept;
    void await_resume() const {}
};


template <typename T> struct generator {
  struct promise_type {
    T current_value;
    suspend_never_bool yield_value(T value) {
      current_value = value;
      return {};
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    generator get_return_object() { return generator{this}; };
    void unhandled_exception() { std::terminate(); }
    void return_value(T v) {  current_value = v; }
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

