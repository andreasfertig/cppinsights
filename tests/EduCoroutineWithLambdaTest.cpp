// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <exception> // std::terminate
#include <new>
#include <utility>

struct generator {
    struct promise_type {
        int                 current_value;
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

    generator(generator&& rhs)
    : p{std::exchange(rhs.p, nullptr)}
    {}

    ~generator()
    {
        if(p) { p.destroy(); }
    }

private:
    explicit generator(promise_type* p)
    : p(std::coroutine_handle<promise_type>::from_promise(*p))
    {}

    std::coroutine_handle<promise_type> p;
};

template<typename _AwrT>
void SyncAwait(_AwrT&& a)
{
    auto asyncLambda = [&]() -> generator { co_await a; };

    asyncLambda();
}

void f()
{
    SyncAwait(std::suspend_always{});
}

