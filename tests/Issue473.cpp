// cmdline:-std=c++20

#include <type_traits>

template <typename T>
struct remove_all_pointers : std::conditional_t<
    std::is_pointer_v<T>,
    remove_all_pointers<std::remove_pointer_t<T>>,
    std::type_identity<T>
> {};

int main() {
    static_assert(std::is_same_v<int, remove_all_pointers<int *>::type>);
}
