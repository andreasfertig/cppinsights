#define INSIGHTS_USE_TEMPLATE

void g()
{
}

template<typename T>
void (&&f(T))()
{
  return g;
}

int main()
{
    long l;
    f(l);

    int i;
    f(i);
}
