int Open() { return 0; }
int Write() { return 0; }
#define SUCCESS 1


auto Foo()
{
    // normal if but with an DeclStmt. No rewrite expected
    if( auto ret = Open() )
    {
        if(  SUCCESS != ret ) {
            return ret;
        }
    }
    // ...
    
    return SUCCESS;
}




int main()
{
    Foo();
}


