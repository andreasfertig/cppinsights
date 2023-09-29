// cmdline:-std=c++20

template<
    class T,
    class Container,
    class _Compare
> class priority_queue
{
    public:
    priority_queue(){}
};


void f()
{
    priority_queue<int,
                        float,
                        decltype([]() { return false; })> min_heap;
}

