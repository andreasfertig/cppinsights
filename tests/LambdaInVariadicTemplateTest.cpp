#include <iostream>


template<class T, class Case, class ...Cases>
decltype(auto) match(T&& value, const Case& _case, const Cases&... cases) {
	return 1;
}


template<class T>
decltype(auto) test(T&& value) {
    return match(value
		,[](std::string value)	{ std::cout <<"This is string "; return value + " Hi!"; }
		,[](int i)				{ std::cout << "This is int ";	 return i * 100; }
		,[](auto a)				{ std::cout << "This is default ";return 0; }
	);
}

int main() {
    std::cout << test(200) << std::endl;
    std::cout << test("RR") << std::endl;          // because const char*
    std::cout << test(std::string{"ARR"}) << std::endl; 
    std::cout << test(2.0f) << std::endl; 
}

