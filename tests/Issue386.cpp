#include <utility>

namespace tnt
{
    // vector
    template <typename T, std::size_t N>
    struct vector final
    {
      static_assert(N > 0);
      
        template <typename U, std::size_t S>
        constexpr vector& operator=(vector<U, S> &&rhs) noexcept
        {
            if (this != &rhs)
                [this, rhs = std::move(rhs)]
                    <std::size_t... I>(std::index_sequence<I...>) noexcept {
                    ((data[I] = std::exchange(rhs.data[I], U(0))), ...);
                }(std::make_index_sequence<S>{});
            return *this;
        }

    private:
        T data[N]{};
    };
} // namespace tnt

int main()
{
}
