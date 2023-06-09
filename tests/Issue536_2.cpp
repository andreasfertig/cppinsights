// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <utility>

struct ClassWithCoro;

struct my_resumable {
  struct promise_type {
    promise_type(const ClassWithCoro& q1, int& q2) {}
    my_resumable get_return_object() {
      return my_resumable(my_resumable::handle_type::from_promise(*this));
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  
  using handle_type = std::coroutine_handle<promise_type>;
  my_resumable(handle_type h) : m_handle(h) {}
  ~my_resumable() { if (m_handle) m_handle.destroy(); }
  my_resumable(my_resumable&& other) = delete;
  handle_type m_handle;
};

struct ClassWithCoro {
  my_resumable coro(int x) const {
    co_return;
  }
};
