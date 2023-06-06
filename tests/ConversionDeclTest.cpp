namespace test {
template<class T = void>
struct coroutine_handle
{
    operator coroutine_handle<>() { return {}; }
};
}

int main()
{
    test::coroutine_handle<int> a{};

    test::coroutine_handle<> b = a;
}


