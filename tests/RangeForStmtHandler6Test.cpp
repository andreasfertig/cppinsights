int main()
{
    int ar[]{1,2,3,4,5};

    int cnt{0};

    for(int pos=0; const auto v: ar){
        cnt += v;
        pos++;
    }

    for( const auto v: ar){
        cnt += v;
    }
    
    return cnt;
}
