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
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    generator get_return_object() { return generator{this}; };
    void unhandled_exception() { std::terminate(); }
    void return_value(int v) { current_value = v; }
    
    // gives us getReturnStmtOnAllocFailure
    static generator get_return_object_on_allocation_failure(){
      throw std::bad_alloc();   
    }    
  };
    
  // shortening the name
  using coro_handle = std::coroutine_handle<promise_type>;

  bool await_ready() { return false; }
  void await_suspend(coro_handle waiter) {
    waiter.resume();
  }
  auto await_resume() {  
    return p.promise().current_value;
  }

  generator(generator &&rhs) : p{std::exchange(rhs.p, nullptr)} {}
  ~generator() { if (p) { p.destroy(); } }

private:
  explicit generator(promise_type* _p)
      : p{coro_handle::from_promise(*_p)} {}

  coro_handle p;
};

generator simpleReturn(int v ) {
    co_return v;
}


generator additionAwaitReturn(int v ) {
    co_return co_await simpleReturn(v) + co_await simpleReturn(v+1);
}

generator additionAwaitReturn2(int v ) {
    co_return co_await simpleReturn(v) + co_await simpleReturn(v+1) + co_await simpleReturn(v+2);
}

generator additionAwaitReturnWithInt(int v ) {
    // Here we look at an example, where __f->__promise.return_value( contains the two other coroutine expressions with
    // a +. Backtracking is required.
    // __f->__promise.return_value(__f->__promise_10_24 + __f->__promise_10_51);
    co_return co_await simpleReturn(v) + co_await simpleReturn(v+1) + 47;
}


int main() {
  auto a = additionAwaitReturn(2);
  auto b = additionAwaitReturn2(2);
  auto c = additionAwaitReturnWithInt(6);
}
