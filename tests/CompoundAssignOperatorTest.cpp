int main()
{
    char c = 1;
    short s = 3;
    int i = 2;
    float f = 2.9f;
    double d = 3.1;

    // char
    c += c;
    c += s;
    c += i;
    c += f;
    c += d;

    // short
    s += c;
    s += s;
    s += i;
    s += f;
    s += d;

    // int
    i += c;
    i += s;
    i += i;
    i += f;
    i += d;    

    // float
    f += c;
    f += s;
    f += i;
    f += f;
    f += d;        

    // double
    d += c;
    d += s;
    d += i;
    d += f;
    d += d;            
}
