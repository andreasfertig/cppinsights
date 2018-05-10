#define HELLO 0x2

int main()
{
    unsigned int x = 0x03;

    if( x & HELLO ) {}

    if( (x & HELLO) == HELLO) {}
}
