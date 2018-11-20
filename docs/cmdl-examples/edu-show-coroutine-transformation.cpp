#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>

namespace std {
using namespace std::experimental;
}
#else
#error "No coroutine header"
#endif

#include <cstdio>
#include <exception>
#include <new>

struct generator
{
    struct promise_type
    {
        int current_value{};

        std::suspend_always yield_value(int value)
        {
            current_value = value;
            return {};
        }

        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        generator           get_return_object() { return generator{this}; };
        void                unhandled_exception() { std::terminate(); }
        void                return_value(int v) { current_value = v; }
    };

    generator(generator const&) = delete;
    generator(generator&& rhs)
    : p{std::exchange(rhs.p, nullptr)}
    {
    }

    ~generator()
    {
        if(handle) {
            handle.destroy();
        }
    }

    void run()
    {
        if(not handle.done()) {
            handle.resume();
        }
    }

    auto value() { return handle.promise().current_value; }

private:
    explicit generator(promise_type* p)
    : handle{std::coroutine_handle<promise_type>::from_promise(*p)}
    {
    }

    std::coroutine_handle<promise_type> handle;
};

generator fun()
{
    printf("Hello,");

    co_yield 4;

    printf("C++ Insights.\n");

    co_return 2;
}

int main()
{
    auto s = fun();

    s.run();

    printf("value: %d\n", s.value());

    s.run();

    printf("value: %d\n", s.value());
}
