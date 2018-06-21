void FloatingCast(double d) {}
void IntegralToBoolean(bool b) {}

// CK_UserDefinedConversion
struct A { operator int() { return 2;} }; 

int main()
{
    float f = 1.0f;

    FloatingCast(f);

    int i = 1;
    IntegralToBoolean(i);

    int ii = int(A());
}
