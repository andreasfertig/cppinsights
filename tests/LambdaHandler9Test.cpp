#include <cstdio>

int main()
{
    auto l = [&]() {
        volatile char buffer[10];
        for(auto& c: buffer) {
            c = 1;
        }
    };

    l();

    auto w = [&]() {
        volatile char buffer[10];
        int i=0;
        while(i < sizeof(buffer)) {
            buffer[i++] = 1;
        }
    };

    w();


    auto s = [&](int x) {

        switch(x) {
            case 2: printf("is 2\n"); break;
            default: return;
        }
   };

    s(4);


    auto f = [&]() {
        volatile char buffer[10];
        for(int i= 0; i < sizeof(buffer); ++i) {
            buffer[i] = 1;
        }
    };

    f();

    auto f2 = [&]() {
        volatile char buffer[10];
        int i=0;
        for(; i < sizeof(buffer); ++i) {
            buffer[i] = 1;
        }
    };

    f2();
}
