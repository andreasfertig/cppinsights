class Test
{
public:
    virtual void Pure() = 0;
};

class West : public Test
{
public:
    void Pure() override {}
};

int main()
{
    West w;
    w.Pure();
}
