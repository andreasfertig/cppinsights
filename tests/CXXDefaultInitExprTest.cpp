template<typename T, bool array>
class Alloc
{
    const int z;
    T* data{nullptr};    
    const bool x{false};
    
public:
    Alloc() : z{2}
    {
    }
};

int main()
{
    Alloc<int, false> a;
    Alloc<char, true> b;
}
