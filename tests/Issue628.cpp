// cmdline:-std=c++2c
// cmdlineinsights:-edu-show-coroutine-transformation -edu-show-cfront

#include <iostream>
#include <coroutine>
#include <thread>
#include <queue>
#include <functional>

std::queue<std::function<bool()>> task_queue;

struct sleep {
    sleep(int n) : delay{n} {}

    constexpr bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) const noexcept {
        auto start = std::chrono::steady_clock::now();
        task_queue.push([start, h, d = delay] {
            if (decltype(start)::clock::now() - start > d) {
                h.resume();
                return true;
            } else {
                return false;
            }
        });
    }

    void await_resume() const noexcept {}

    std::chrono::milliseconds delay;
};


struct Task {
    struct promise_type {
        promise_type() = default;
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; } 
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
    };
};

Task foo() noexcept {
    std::cout << "1. hello from foo1" << std::endl;
    for (int i = 0; i < 10; ++i) {
        co_await sleep{10};
        std::cout << "2. hello from foo1" << std::endl;
    }
}

//call foo
int main() {
    foo();
}
