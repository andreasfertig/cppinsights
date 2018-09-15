#define INSIGHTS_USE_TEMPLATE

int arr[5][3];

template<typename T>
int (&f(T i))[3]
{
  return arr[i];
}


int main()
{
    long l;
    f(l);

    int i;
    f(i);
}
