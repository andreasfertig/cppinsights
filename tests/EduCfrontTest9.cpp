// cmdlineinsights:-edu-show-cfront

class Test
{
    int mX;

public:
    bool operator==(int rhs) const { return rhs == mX; }
};

bool b = Test{} == 3;
