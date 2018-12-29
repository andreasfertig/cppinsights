#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <typeinfo>



// ====================  Copy paste this ==============================================================================
#include <tuple>
#include <utility>

template <typename T>
struct FunctionArgs : FunctionArgs<decltype(&T::operator())> {};

template <typename R, typename... Args>
struct FunctionArgsBase {
    using args = std::tuple<Args...>;
	using arity = std::integral_constant<unsigned, sizeof...(Args)>;
	using result = R;
};

template <typename R, typename... Args>
struct FunctionArgs<R(*)(Args...)> : FunctionArgsBase<R, Args...> {};
template <typename R, typename C, typename... Args>
struct FunctionArgs<R(C::*)(Args...)> : FunctionArgsBase<R, Args...> {};
template <typename R, typename C, typename... Args>
struct FunctionArgs<R(C::*)(Args...) const> : FunctionArgsBase<R, Args...> {};

// forward declarations
template<class T, class Case, class ...Cases>
decltype(auto) match(T&& value, const Case& _case, const Cases&... cases);
template<class T, class Case>
decltype(auto) match(T&& value, const Case& _case);

namespace details {
	template<class T, class Case, class ...OtherCases>
	decltype(auto) match_call(const Case& _case, T&& value, std::true_type, const OtherCases&... other) {
		return _case(std::forward<T>(value));
	}

	template<class T, class Case, class ...OtherCases>
	decltype(auto) match_call(const Case& _case, T&& value, std::false_type, const OtherCases&... other) {
		return match(std::forward<T>(value), other...);
	}
}

template<class T, class Case, class ...Cases>
decltype(auto) match(T&& value, const Case& _case, const Cases&... cases) {
    using namespace std;
    using args = typename FunctionArgs<Case>::args;
	using arg = tuple_element_t<0, args>;
	using match = is_same<decay_t<arg>, decay_t<T>>;
	return details::match_call(_case, std::forward<T>(value), match{}, cases...);
}


// the last one is default
template<class T, class Case>
decltype(auto) match(T&& value, const Case& _case) {
	return _case(std::forward<T>(value));
}

// ==================================================================================================



template<class T>
decltype(auto) test(T&& value) {
    return match(value
		,[](std::string value)	{ std::cout << "This is string "; return value + " Hi!"; }
		,[](int i)				{ std::cout << "This is int ";	 return i * 100; }
		,[](auto a)				{ std::cout << "This is default ";return 0; }
	);
}

int main() {
    std::cout << test(200) << std::endl;
    std::cout << test("RR") << std::endl;          // because const char*
    std::cout << test(std::string{"ARR"}) << std::endl; 
	return 0;
}

