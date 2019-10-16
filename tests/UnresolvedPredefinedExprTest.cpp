#define INSIGHTS_USE_TEMPLATE

// Put the namespace here to force rewriting of the primary template
namespace UnresolvedLookupExprTest {
  template<typename T>
  class Test
  {
  public:
      Test()
      {
        const char* f = __func__; // UnresolvedLookupExpr
      }
  };
}

int main()
{
    const char* f = __func__;

    UnresolvedLookupExprTest::Test<int> t;
}
