// cmdline:-std=c++2b
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <iostream>
#include <exception>

template <typename T, typename U>
struct executable { static U execute() { co_await T{}; } };
struct hello_logic  {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept { }
    void await_resume() const noexcept { std::cout << "Hello, world" << std::endl; }
};
struct hello_world : executable<hello_logic, hello_world> {
    struct promise_type {
        auto get_return_object() { return hello_world(this); }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { }
    };
    using coro_handle = std::coroutine_handle<promise_type>;
    hello_world(promise_type* promise) : handle_(coro_handle::from_promise(*promise)) { }
private:
    coro_handle handle_;
};

int main() { hello_world::execute(); }
