int main()
{
   int w = [&]() {
        using mType = int;

        typedef int xType;

        typedef void (*SignalHandler)(int signum);
        
        mType x = 1;
        xType y = 2;

        return x + y;
    }();

#if 0
   []() {
        using fp = int (*)(int, char);
      
        fp fd   = [](auto a, auto b){ return a + b; };
   }();
#endif
}
