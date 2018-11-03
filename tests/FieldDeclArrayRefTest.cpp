#define INSIGHTS_USE_TEMPLATE

static int x[2]{1,2};

template <typename T, unsigned int SIZE>
class array
{
private:
    T (&data)[SIZE];
public:
    array() : data(x) { }
    array(T (&reference)[SIZE]) : data(reference) { }
};

int main()
{
    array<int, 2> a;
    array<int, 2> ab{x};
}
