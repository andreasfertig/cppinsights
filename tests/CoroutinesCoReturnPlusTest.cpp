// cmdline:-std=c++2a
#include <experimental/coroutine>

namespace stdx = std::experimental;
 
struct generator {
  struct promise_type {
    int current_value;
    stdx::suspend_always yield_value(int value) {
      this->current_value = value;
      return {};
    }
    
    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_always final_suspend() noexcept { return {}; }
    generator get_return_object() { return generator{this}; };
    void unhandled_exception() { std::terminate(); }
    void return_value(int value) { }

    // gives us getReturnStmtOnAllocFailure
    static generator get_return_object_on_allocation_failure(){
      throw std::bad_alloc();   
    }    
  };

    
  // shortening the name
  using coro_handle = stdx::coroutine_handle<promise_type>;

  bool await_ready() { return false; }
  void await_suspend(coro_handle waiter) {
    waiter.resume();
  }
  auto await_resume() {  
    return p.promise().current_value;
  }

  generator(generator const&) = delete;
  generator(generator &&rhs) : p(rhs.p) { rhs.p = nullptr; }

  ~generator() {
    if (p)
      p.destroy();
  }

private:
  explicit generator(promise_type *p)
      : p(coro_handle::from_promise(*p)) {}

  coro_handle p;
};

generator simpleReturn(int v ) {
    co_return v;
}



generator additionAwaitReturn(int v ) {
    // Here we look at an example, where __f->__promise.return_value( contains the two other coroutine expressions with
    // a +. Backtracking is required.
    // __f->__promise.return_value(__f->__promise_10_24 + __f->__promise_10_51);
    co_return co_await simpleReturn(v) + co_await simpleReturn(v) + co_await simpleReturn(v+1);
}

generator awaitReturn(int v ) {
    // Here we look at an example, where __f->__promise.return_value( contains the two other coroutine expressions with
    // a +. Backtracking is required.
    // __f->__promise.return_value(__f->__promise_10_24 + __f->__promise_10_51);
    co_return co_await simpleReturn(v+41);
}

generator bracedReturn(int v ) {
    // Here we look at an example, where __f->__promise.return_value( contains the two other coroutine expressions with
    // a +. Backtracking is required.
    // __f->__promise.return_value(__f->__promise_10_24 + __f->__promise_10_51);
    co_return { v };
}

int main() {
  auto sr = simpleReturn(3);

  auto aar = additionAwaitReturn(2);
  
  auto ar = awaitReturn(44);

  auto br = bracedReturn(5);
}
