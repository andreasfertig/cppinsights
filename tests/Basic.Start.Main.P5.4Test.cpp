int main(int argc, const char* argv[])
{
    if(0 == argc) { return 1; }
    
    int x = []{ return 2; }(); // a return statement in main that does not return from main

    // implicit return 0 added here
}

