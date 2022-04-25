// cmdlineinsights:-edu-show-cfront

int main()
{
    int*         i  = new int;
    long double* d  = new long double(22.4);
    double*      db = new double{22.4};

    delete i;
    delete db;
}
