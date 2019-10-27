// This wrapper is needed, because currently the base template is not rewritten if it has a TU as direct parent.
class CXXFoldExprTest
{
public:
    template<typename... Ts>
    auto UnaryRightFold(const Ts&... ts) {
        return (ts / ...);
    }

    template<typename... Ts>
    auto UnaryLeftFold(const Ts&... ts) {
        return (... / ts);
    }

    template<typename... Ts>
    auto BinaryRightFold(const Ts&... ts) {
        return (ts/ ... / 2);
    }

    template<typename... Ts>
    auto BinaryLeftFold(const Ts&... ts) {
        return (2 / ... /ts);
    }
};

int main()
{
    CXXFoldExprTest cxxFoldExprTest{};

    cxxFoldExprTest.UnaryRightFold(2,3,4,5);
    cxxFoldExprTest.UnaryLeftFold(2,3,4,5);
    cxxFoldExprTest.BinaryRightFold(2,3,4,5);
    cxxFoldExprTest.BinaryLeftFold(2,3,4,5);
}

