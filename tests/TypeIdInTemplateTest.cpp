#include <string>

template <typename T>
class Foo{
public:    
    std::string Get() {
        std::string typeId{typeid(T).name()};
        return typeId;
    }

};



int main()
{
  Foo<int> i;

  i.Get();
}

