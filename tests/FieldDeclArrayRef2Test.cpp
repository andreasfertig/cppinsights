template <typename T, unsigned int SIZE>
class array
{
private:
    T (&data)[SIZE];
public:
    array() : data(*new T[1][SIZE]) { }
    array(T (&reference)[SIZE]) : data(reference) { }
};

int main()
{
    array<int, 2> a;
}
