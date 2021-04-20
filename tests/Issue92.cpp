// cmdline:-std=c++2a
#include <experimental/coroutine>

template <typename T>
struct generator
{
	struct promise_type
	{
		T current_value;
		std::experimental::suspend_always yield_value(T value)
		{
			this->current_value = value;
			return {};
		}
		std::experimental::suspend_always initial_suspend() { return {}; }
		std::experimental::suspend_always final_suspend() noexcept { return {}; }
		generator get_return_object() { return generator{ this }; };
		void unhandled_exception() { std::terminate(); }
		void return_void() {}
        // gets us stmt->getReturnStmtOnAllocFailure()
    static auto get_return_object_on_allocation_failure() { return generator{nullptr}; }
	};

    
	struct iterator
	{
		std::experimental::coroutine_handle<promise_type> hco;
		bool done = false;

		iterator(std::experimental::coroutine_handle<promise_type> hco, bool done)
		: hco(hco), done(done) {}

		iterator& operator++()
        {
			hco.resume();
			done = hco.done();
			return *this;
		}

		bool operator==(const iterator&o) const
        {
			return done == o.done;
		}
		bool operator!=(const iterator&o) const { return !(*this == o); }

		const T& operator*() const { return hco.promise().current_value; }
		const T* operator->() const { return &(operator*()); }
	};

	iterator begin()
    {
		p.resume();
		return { p, p.done() };
	}
	iterator end() { return { p, true }; }

	generator(generator&& rhs) : p(rhs.p) { rhs.p = nullptr; }
	~generator()
    {
		if (p)
			p.destroy();
	}

private:
	explicit generator(promise_type* p)
	: p(std::experimental::coroutine_handle<promise_type>::from_promise(*p)) {}

	std::experimental::coroutine_handle<promise_type> p;
};

generator<uint32_t> fibonaccis()
{
    uint32_t a = 0, b = 1;
	while (true)
    {
		co_yield b;
		std::tie(a, b) = std::make_pair(b, a+b);
		co_yield a;
	}
}

generator<uint32_t> take(generator<uint32_t>& g, uint32_t end)
{
	uint32_t i = 0;
	for (auto e : g)
    {
		if (i >= end)
			break;

		co_yield e;
		++i;
		co_yield i;
	}
}

uint32_t arr[10];
int main()
{
	auto g = fibonaccis();
	uint32_t i = 0;
	for (uint32_t e : take(g, 10))
		arr[i++] = e;
}
