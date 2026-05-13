struct PtrToMemTest
{
    int value;
};


int PtrToMemTest::*fixed = &PtrToMemTest::value;

auto deduced = &PtrToMemTest::value;
