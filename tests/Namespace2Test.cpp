class Test
{
public:
    template<typename T> 
    class my_array {};
 
    // two type template parameters and one template template parameter:
    template<template<typename> typename C = my_array>
    class Map
    {
    };    
};


int main(){
    Test::Map<> a;
}
