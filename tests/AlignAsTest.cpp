struct alignas(16) AlignedStruct
{
    float data[4];
};
 
alignas(128) char alignedArray[128];
 
template <typename... Ts>
struct AlignedStructWithPack
{
   char data alignas(Ts...);
};
 
int main()
{
    AlignedStructWithPack<int, char, double> a;
}
