#include <chrono>

template <typename T>
void Test(){
    typedef std::chrono::duration<double, std::ratio<60>> simple;
    typedef std::chrono::duration<double, std::ratio<60*60*24*365>> withCalculation; // contains an implicit cast in the template definition
}
  

int main()
{
    typedef std::chrono::duration<double, std::ratio<60>> simple;
    typedef std::chrono::duration<double, std::ratio<60*60*24*365>> withCalculation; // contains a calculation we will see the result

    Test<int>();
}

