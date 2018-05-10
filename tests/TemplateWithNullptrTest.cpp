class A {
public:
void SomeMethod()
{
}
};


template<class T, void(T::*SomeMethod)() = nullptr>
class B {


};

B<A> b1; //OK
//B<A, static_cast<SomeMethod_t>(0)> b2; //OK
B<A, nullptr> b3; //OK
//B<A, 0> b4; // error: could not convert template argument ‘0’ to ‘void (A::*)()’

