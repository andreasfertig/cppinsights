class Test
{
public:
    Test(): mV{} {}

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

    Test& operator++()
    {
        mV++;
        return *this;
    }

    Test operator++(int)
    {
        Test tmp(*this);
        operator++();
        return tmp;  
    }    

    Test& operator+=(const Test& rhs) 
  {              
    mV += rhs.mV;
    return *this; 
  }
 
  operator int() { return mV; }

private:
    int mV;
};


struct S
{
    Test count;
    
};

int main()
{
    S s;
	S s2;
  
    s.count++;

    Test t;
  
	char * raw = reinterpret_cast<char*>(&s+1);
	Test * p =  reinterpret_cast<Test*>(&raw[s.count]); // missing the user-defined converion op here for s.count to int
    *p = (t);
    *p = s2.count;
  
    s.count += sizeof(p);
    s.count += alignof(p);
    s.count += p ? 1 : 2;    
}
