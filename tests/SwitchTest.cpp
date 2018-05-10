const unsigned short FOO_BASE = 2;
const unsigned short FOO = FOO_BASE + 1;

int main()
{
    int x;

    switch(x) {
        case FOO: return 1;
    }

    switch(int y = 1) {
        case FOO: return 1;
    }   

    switch(int y = 1; y++) {
        case FOO: return 1;
    }   
}
