// cmdline:-std=c++2a
template<typename T>
concept FWD = requires { T{}; T(); };

int main()
{
    FWD auto x = 4;
}
