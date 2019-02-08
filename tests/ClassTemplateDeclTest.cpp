// A test for future reference. As C++ Insights does currently not match a TemplateDecl
// it is wrapped in a CXXRecordDecl which is matched. This should ensure that the code for
// that stays alive to the day all Decl's are matched.
class Test
{
public:
    template<typename T>
    class Tmpl
    {
        T m;
    };
};

int main()
{
    Test::Tmpl<int> x;
}
