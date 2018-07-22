struct Base
{
    Base();
    Base(const Base &);
    ~Base();
};

Base::Base() = default;
Base::Base(const Base &) = default;
Base::~Base() = default;
