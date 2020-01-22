// cmdline:-std=c++2a
template<typename T>
concept default_constructible = requires { T{}; T(); };


// N4842 p. 181 9.3.3.5 /19: An abbreviated function template can have a template-head. The invented template-parameters are appended to the template-parameter-list after the explicitly declared template-parameters. [Example:
template<typename T>
void fun(T t, default_constructible auto x)
{
}

class AutoAsParameter {
public:
    template<typename T>
    void fun(T t, default_constructible auto x)
    {
    }
};

class ConstraintTemplateTypeParameter
{
public:
    template<typename T, default_constructible X>
    void fun(T t, X x)
    {
    }
};

int main()
{
    int x = 4;
    fun(2, x);


    AutoAsParameter a{};
    a.fun(2,x);


    ConstraintTemplateTypeParameter c{};
    c.fun(2,x);
}
