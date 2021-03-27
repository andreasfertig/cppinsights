// cmdline:-std=c++2a
#include <functional>
#include <iostream>

namespace tnt
{
    template <typename T>
    [[nodiscard]] constexpr std::string_view type_name() noexcept
    {
        // thx Boost.UT authors
        // https://github.com/boost-ext/ut/blob/3b05dca6a629497910cf8e92aebcaead0124c8b4/include/boost/ut.hpp#L228
#if defined(_MSC_VER) and not defined(__clang__)
        return {__FUNCSIG__ + 89, sizeof(__FUNCSIG__) - 106};
#elif defined(__clang__)
        return {__PRETTY_FUNCTION__ + 39, sizeof(__PRETTY_FUNCTION__) - 41};
#elif defined(__GNUC__)
        return {__PRETTY_FUNCTION__ + 54, sizeof(__PRETTY_FUNCTION__) - 105};
#endif
    }

    template <auto A>
    [[nodiscard]] constexpr std::string_view value_name() noexcept
    {
        // thx Boost.UT authors
        // https://github.com/boost-ext/ut/blob/3b05dca6a629497910cf8e92aebcaead0124c8b4/include/boost/ut.hpp#L228
#if defined(_MSC_VER) and not defined(__clang__)
        return {__FUNCSIG__ + 89, sizeof(__FUNCSIG__) - 106};
#elif defined(__clang__)
        return {__PRETTY_FUNCTION__ + 40, sizeof(__PRETTY_FUNCTION__) - 42};
#elif defined(__GNUC__)
        return {__PRETTY_FUNCTION__ + 60, sizeof(__PRETTY_FUNCTION__) - 111};
#endif
    }

    template <typename>
    struct wrapper;

    template <typename T>
    struct printer final
    {
        explicit constexpr printer(T const& ref) noexcept
            : data{std::addressof(ref)} {}

        printer(T const&&) = delete;

        template <typename R, typename... Args>
        void operator()(std::string_view name, R (T::*ptr)(Args...))
        {
          	std::cout << "\t" << type_name<R>() << ' ' << name << " = " << std::invoke(ptr, data) << ";\n";
        }

        template <typename R>
        void operator()(std::string_view name, R T::*ptr)
        {
          	std::cout << "\t" << type_name<R>() << ' ' << name << " = " << std::invoke(ptr, data) << ";\n";
        }

    private:
        const T* data;
    };

    template <typename T>
        requires requires { &tnt::wrapper<T>::template operator()<printer<T>>; }
    struct serializer final
    {
        template <typename Vis>
        static constexpr void get_visit(Vis &&vis)
        {
            wrap(std::forward<Vis>(vis));
        }

        static constexpr wrapper<T> wrap;
    };
}

struct my_type final
{
    int a;
    std::string_view b;
};

template <>
struct tnt::wrapper<my_type> final
{
    template <typename Vis>
    constexpr void operator ()(Vis &&vis) const
    {
        vis("a", &my_type::a);
        vis("b", &my_type::b);
    }
};

int main()
{
    constexpr my_type t{.a = 10, .b = "42"};
  	std::cout << "struct " << tnt::type_name<my_type>() << " "
      << (std::is_final_v<my_type> ? "final" : "") << "\n{\n";
    tnt::printer<my_type> p{t};
    tnt::serializer<my_type>::get_visit(p);
	std::cout << "};\n";
}
