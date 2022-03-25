// cmdlineinsights:-edu-show-padding

struct Empty{ };

struct NoUniqueAddrTest
{
    int                         i;
    [[no_unique_address]] Empty e;
};

