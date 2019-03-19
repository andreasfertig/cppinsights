#include <cstdio>

int main()
{
    int x = 0;

    for(int i = [&]{ return x*2; }(); i < 20; ++i) {
        x += i;
    }

    for(int i = [&]{ return x*2; }(), z = [&]{ return x*2; }(); i < 20; ++i) {
        x += i;
    }

    for([]{printf("started\n");}(); [&]{ return --x; }(); []{printf("after\n");}())
    {}


    for([]{printf("started\n");}(),
        []{printf("started\n");}(); [&]{ return --x; }(); []{printf("after\n");}())
    {}
    
}


