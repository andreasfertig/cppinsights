template<typename T, bool array>
class Alloc
{
    T* data;
    
public:
    Alloc() {
        if( array ) {
            data = new T[10];
            data = new T[10]{1,2,3};
        } else {
            data = new T;
            data = new T(2);
            data = new T{2};
        }
    }

    ~Alloc() {
        if( array ) {
            delete[] data;
        } else {
            delete data;
        }
    }
};

int main()
{
    Alloc<int, false> a;
    Alloc<char, true> b;
}
