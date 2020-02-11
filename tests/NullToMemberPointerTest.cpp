// cmdlineinsights:--show-all-implicit-casts
class NullToMemberPtrTest
{
public:
    int Fun(int) { return 1; }

    int mMember;
};

int main()
{
    int NullToMemberPtrTest::*mMember1 = nullptr;

    int NullToMemberPtrTest::*mMember2 = 0;

    int (NullToMemberPtrTest::*Func1)(int) = nullptr;

    int (NullToMemberPtrTest::*Func2)(int) = 0;
}

