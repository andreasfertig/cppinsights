int Open() { return 0; }
int Write() { return 0; }
#define SUCCESS 1
#define FAIL 2


auto Foo()
{
    switch( auto ret = Open(); ret )
    {
        case SUCCESS: return 1;
        case FAIL: return 0;
    }

    return -1;
}

int main()
{
    Foo();
}

