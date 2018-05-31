#include <cstdio>

int main() {
    struct SP
    {
        int x;
        int y;
    };

    SP sp{1, 2};

    auto[sx, sy] = sp;

    class CP
    {
    public:
        int x;
        int y;
    };

    CP cp{1,2};

    auto [ c1, c2 ] = cp;    

    printf("c1: %d c2: %d\n", cp.x, cp.y);
    ++c1;
    ++c2;
    printf("c1: %d c2: %d\n", cp.x, cp.y);

    auto& [ c3, c4 ] = cp;    
    
    printf("c3: %d c4: %d\n", cp.x, cp.y);
    ++c3;
    ++c4;
    printf("c3: %d c4: %d\n", cp.x, cp.y);
}
