struct DefaultedCtorAndConstexpr
{
  DefaultedCtorAndConstexpr()= default;
};

int main()
{
    DefaultedCtorAndConstexpr t;
}
