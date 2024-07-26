// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <cstdio>
#include <exception>  // std::terminate
#include <iostream>
#include <list>
#include <new>
#include <string_view>
#include <utility>

using namespace std::string_literals;
using namespace std::string_view_literals;

struct Task {
  struct promise_type {
    Task               get_return_object() noexcept { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void               return_void() noexcept {}
    void               unhandled_exception() noexcept {}
  };
};

struct Scheduler;

struct awaiter : std::suspend_always {
  Scheduler* _sched;

  explicit awaiter(Scheduler& sched)
  : _sched{&sched}
  {}
  void await_suspend(std::coroutine_handle<> coro) const noexcept;
};

struct Scheduler {
  std::list<std::coroutine_handle<>> _tasks{};

  bool schedule()
  {
    auto task = _tasks.front();
    _tasks.pop_front();

    if(not task.done()) { task.resume(); }

    return not _tasks.empty();
  }

  auto suspend() { return awaiter{*this}; }
};

void awaiter::await_suspend(std::coroutine_handle<> coro) const noexcept
{
  _sched->_tasks.push_back(coro);
}

Task taskA(Scheduler& sched)
{
  std::cout << "Hello, from task A\n"sv;

  co_await sched.suspend();

  static std::string res{"a is back doing work\n"s};
  std::cout << res;
}

int main()
{
  Scheduler scheduler{};

  taskA(scheduler);

  while(scheduler.schedule()) {}
}

