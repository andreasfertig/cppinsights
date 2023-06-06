// cmdline:-std=c++20
// cmdlineinsights:-edu-show-coroutine-transformation

#include <coroutine>
#include <utility>

struct my_resumable {
    struct promise_type {
    promise_type() = default;
    promise_type(int& a, char& b, double& c) { }

    my_resumable get_return_object() {
        auto h = my_resumable::handle_type::from_promise(*this);
        return my_resumable(h);
    }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}

    void unhandled_exception() {}
};


    using handle_type = std::coroutine_handle<promise_type>;
    my_resumable() = default;
    my_resumable(handle_type h) : m_handle(h) {}
    ~my_resumable() { if (m_handle) m_handle.destroy(); }
    my_resumable(my_resumable&& other) noexcept : m_handle(std::exchange(other.m_handle, nullptr)) {}
    my_resumable& operator=(my_resumable&& other) noexcept {
        m_handle = std::exchange(other.m_handle, nullptr);
        return *this;
    }
    handle_type m_handle;
};

my_resumable coro(int a, char b, double c) {
    co_return;
}
