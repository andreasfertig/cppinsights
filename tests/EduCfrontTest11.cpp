// cmdlineinsights:-edu-show-cfront

int main()
{
    static_assert(sizeof(int) != 0);

    static_assert(sizeof(int) != 0, "Just a test");

    int* p = nullptr;

    typedef int myInt;
    // using myOtherInt = int;
}
