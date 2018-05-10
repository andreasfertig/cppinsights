// Simulate the inline namespace behaviour of clangs libc++ where
// things like std::string are in a inlined namespace
namespace mine {
    inline namespace __1 {
        static const int x = 1;
    }
}

int main()
{
    []() {
        int x = mine::__1::x; 

    }();
}
