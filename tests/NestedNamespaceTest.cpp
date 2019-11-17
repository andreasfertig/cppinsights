struct One
{
    static const int o{};

    struct Two
    {
        struct Three
        {
            static const int c = 4;
        };

        static const int b = Three::c;
        static const int d = o;
    };

    static const int a = Two::d;
    static const int x = Two::Three::c;
};

int main()
{
    One one;
    One::Two two;
    One::Two::Three three;
}
