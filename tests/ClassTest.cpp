class Test
{
public:
    Test(int v)
    : mV{v}
    {}

    Test(Test& v)
    : mV{v.mV}
    {}

  
    Test& operator=(const Test& other)
    {
        mV = other.mV;
        return *this;
    }

#if 0  
    Test& operator=( Test&& other)
    {
        mV = other.mV;
        return *this;
    }
#endif
  
#if 0  
    Test& operator=(Test arg) noexcept // copy/move constructor is called to construct arg
    {
        mV = arg.mV;
        return *this;
    }
  #endif

    Test& operator++()
    {
        mV++;
        return *this;
    }

    Test operator++(int)
    {
        Test tmp(*this); // copy
        operator++(); // pre-increment
        return tmp;   // return old value
    }    

    Test& operator+=(const Test& rhs) // compound assignment (does not need to be a member,
  {                           // but often is, to modify the private members)
    mV += rhs.mV;
    return *this; // return the result by reference
  }
 
  operator int() { return mV; }

private:
    int mV;
};

int main()
{
    Test t(2);
    

    t = 3 *2;

	Test tt(4);

  	t += tt * 2;
  
}

