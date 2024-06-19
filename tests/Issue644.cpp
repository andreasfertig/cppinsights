// cmdline:-std=c++20

int main(int argc, char* argv[])
{
    if(argc) [[likely]] 
        return 5;
    else 
        return 8;

    if(argc) [[likely]] {
        return 5;
    } else {
        return 8;
    }


    switch(argc) {
        case 1: return 6;
        [[likely]] case 2: return 9;
    }

}
