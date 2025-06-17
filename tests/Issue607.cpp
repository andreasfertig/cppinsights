// cmdlineinsights:-edu-show-cfront
namespace X
{
  int initInNamespace;
}

extern int initDueToExtern;

int unInitGlobal;
int initGlobalWitValue = 3;

static int initGlobalStatic;

struct A{};

A initGlobalCtor;

namespace
{
    int initInAnonNamespace;
    A initCtorInAnonNamespace;
}

int main()
{
  ++X::initInNamespace;
}
