// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation


// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/n4736.pdf
#include <coroutine>
#include <exception> // std::terminate
#include <new>
#include <utility>
#include <chrono>
#include <iostream>

template<typename T>
struct my_future
{
	struct promise_type
	{
		std::suspend_always initial_suspend() { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
        auto get_return_object() { return my_future{}; }
		void unhandled_exception() { std::terminate(); }
		void return_void() {}
	};

    
    bool await_ready();
    void await_suspend(std::coroutine_handle<>);
    T    await_resume();
};

template<class Rep, class Period>
auto operator co_await(std::chrono::duration<Rep, Period> d)
{
    struct awaiter
    {
        std::chrono::system_clock::duration duration;
        awaiter(std::chrono::system_clock::duration d)
        : duration(d)
        {}

        bool await_ready() const { return duration.count() <= 0; }
        void await_resume() {}
        void await_suspend(std::coroutine_handle<> h) {  }
    };
    
    return awaiter{d};
}
using namespace std::chrono;

my_future<int>  h();
my_future<int> g()
{
    std::cout << "just about go to sleep...\n";
    co_await 10ms;
    std::cout << "resumed\n";
    co_await h();
}

