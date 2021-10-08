// cmdlineinsights:-edu-show-noexcept

void Fun() noexcept(true)
{
    int i = 3;
}

void Fun2() noexcept(false)
{
    int i = 3;
}

void Fun3() noexcept
{
    int i = 3;
}

void Fun4()
{
    int i = 3;
}


