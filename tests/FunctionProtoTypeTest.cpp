template <typename T>
struct FunctionArgs : FunctionArgs<decltype(&T::operator())> {};

template <typename R, typename... Args>
struct FunctionArgs<R(*)(Args...)> {};

