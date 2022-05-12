// cmdline:-std=c++2a
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
    void return_void() {}

    // enable this to have co_return return a value, otherwise use return_void
    //void return_value(T value) { }

    // gives us getReturnStmtOnAllocFailure
    static generator get_return_object_on_allocation_failure(){
      throw std::bad_alloc();   
    }    
  };

  // shortening the name
  using coro_handle = std::coroutine_handle<promise_type>;
  
  struct iterator {


    coro_handle _Coro;
    bool _Done;

    iterator(coro_handle Coro, bool Done)
        : _Coro(Coro), _Done(Done) {}

    iterator &operator++() {
      _Coro.resume();
      _Done = _Coro.done();
      return *this;
    }

    bool operator==(iterator const &_Right) const {
      return _Done == _Right._Done;
    }

    bool operator!=(iterator const &_Right) const { return !(*this == _Right); }
    int const &operator*() const { return _Coro.promise().current_value; }
    int const *operator->() const { return &(operator*()); }
  };

  iterator begin() {
    p.resume();
    return {p, p.done()};
  }

  iterator end() { return {p, true}; }

  generator(generator &&rhs) : p{std::exchange(rhs.p, nullptr)} {}
  ~generator() { if (p) { p.destroy(); } }

private:
  explicit generator(promise_type* _p)
      : p{coro_handle::from_promise(*_p)} {}

  coro_handle p;
};


struct auto_await_suspend {
  bool await_ready();
  template <typename F> 
  auto await_suspend(F) { return false;}
  void await_resume();
};


generator seq(int start) {
  for (int i = start;; ++i) {
    co_await auto_await_suspend{};
  }
}

int main() {
  auto s = seq(3);

  for(auto&& i : s ) {}
}
