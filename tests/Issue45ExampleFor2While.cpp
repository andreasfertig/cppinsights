// cmdlineinsights:-alt-syntax-for
int main()
{
    int x = 0;

    for(; x < 20; ++x) {
        x += x;
    }

    for(int i=0; x < 20; ++x) {
        x += i;
    }

    for(int i = 0, y=2, t=4, o=5; i < 20; ++i) {
        x += i;
    }

    for(int *i = &x, *y=&x; i ; ++i) {
        x += *i;
    }

    for(;;);


    char data[5]{};
    for(auto& x : data) {
        x = 2;
    }
}
