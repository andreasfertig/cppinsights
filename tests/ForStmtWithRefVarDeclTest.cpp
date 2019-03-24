// cmdlineinsights:-alt-syntax-for
int main()
{
    int x{};
    
    for(int& z=x, &y=z; true;) {}
}
