int main()
{
    char p[2];
    const auto[x, y] = p;


    volatile char p2[2];
    const auto[x2, y2] = p2;

    char p3[2];
    auto& [x3, y3] = p3;
}
