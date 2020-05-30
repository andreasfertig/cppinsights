namespace test {
    [[maybe_unused]] [[nodiscard]] inline int multipleGnuAttributes();

    [[nodiscard]] int
    forwardDeclWithAttributes();

    int forwardDeclWithAttributes() { return 0; } // impl to former decl


    // C++17:
    [[using gnu: const, always_inline, hot]] [[nodiscard]] int usingAttribute [[gnu::always_inline]] ();

    [[nodiscard]] int twiceTheSameAttributeOnDifferentPlace [[nodiscard]] ();

    [[noreturn]] void noReturn();

    [[deprecated]] void deprecatedFunc();

    [[deprecated("Replaced by bar, which has an improved interface")]] void deprecatedWithMsgFunc();

    void fallThroughAtrrTest(int n)
    {
        switch(n) {
            case 1: [[fallthrough]];
            case 2: break;
        }
    }

    struct [[nodiscard]] nodiscardTest{};
    struct [[nodiscard("just a test")]] nodiscardWithMsgTest{};

    [[maybe_unused]] void maybeUnusedTest([[maybe_unused]] bool b)
    {
        [[maybe_unused]] bool b2 = b;
    }

// C++20
#if 0
int likelyTest(int i) {
    switch(i) {
    [[unlikely]] case 1: return 2;
    [[likely]] case 2: return 1;
    }
    return 2;
}
#endif

    // empty class
    struct Empty
    {
    };

    struct NoUniqueAddrTest
    {
        int                         i;
        [[no_unique_address]] Empty e;
    };

}  // namespace test

int main() {}

