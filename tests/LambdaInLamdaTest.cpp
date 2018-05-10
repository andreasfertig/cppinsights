int main() 
{
    char buffer;
    [&]() {
        
        [&]() {
            buffer = 2;
        }();

    }();

}
