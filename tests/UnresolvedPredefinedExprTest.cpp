#define INSIGHTS_USE_TEMPLATE

template<typename T>
class Test
{
public:
    Test()
    {
        const char* f = __func__; // UnresolvedLookupExpr
    }
};

int main()
{
    const char* f = __func__;

    Test<int> t;
}
