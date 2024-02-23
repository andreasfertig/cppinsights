// cmdlineinsights:-edu-show-cfront

class Alloc
{
    int* data;

public:
    Alloc() { data = new int[10]{1, 2, 3}; }

    ~Alloc() { delete[] data; }
};

int main()
{
    Alloc a;
}
