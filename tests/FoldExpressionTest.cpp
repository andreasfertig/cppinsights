// + - * / % ^ & | = < > << >> += -= *= /= %= ^= &= |= <<= >>= == != <= >= && || , .* ->*

// Tests for some of the 32 operators

template<typename... Args>
constexpr auto plus(Args... args) { return (... + args); }

template<typename... Args>
constexpr auto minus(Args... args) { return (... - args); }

template<typename... Args>
constexpr auto times(Args... args) { return (... * args); }

template<typename... Args>
constexpr auto div(Args... args) { return (... / args); }

template<typename... Args>
constexpr auto mod(Args... args) { return (... % args); }

template<typename... Args>
constexpr auto bitwiseXOr(Args... args) { return (... ^ args); }

template<typename... Args>
constexpr auto bitwiseAnd(Args... args) { return (... & args); }

template<typename... Args>
constexpr auto bitwiseOr(Args... args) { return (... | args); }


// = 

template<typename... Args>
constexpr auto lt(Args... args) { return (... < args); }

template<typename... Args>
constexpr auto gt(Args... args) { return (... > args); }


template<typename... Args>
constexpr auto logicalAnd(Args... args) { return (... && args); }

template<typename... Args>
constexpr auto logicalOr(Args... args) { return (... ||args); }

template<typename... Args>
constexpr auto comma(Args... args) { return (... , args); }

int main()
{
    static_assert(6 == plus(1,2,3));
    static_assert(1 == minus(6,2,3));
    static_assert(18 == times(3,2,3));
    static_assert(3 == div(18,2,3));
    static_assert(0 == mod(18,4,2));
    static_assert(0b00'011 == bitwiseXOr(0b11'101, 0b11'010, 0b00'100));
    static_assert(0b00'010 == bitwiseAnd(0b11'010, 0b11'010, 0b00'010));
    static_assert(0b11'111 == bitwiseOr(0b11'001, 0b11'010, 0b00'100));

// = 

    static_assert(false == lt(1,2,0));
    static_assert(false == gt(1,2,1));


    static_assert(false == logicalAnd(true, true, true, false));
    static_assert(true == logicalOr(true, true, true, false));

    static_assert(4 == comma(1,2,3,4));

    // Verify that we do not introduce parens here.
    int x = 4;
    x /= 2;
}

