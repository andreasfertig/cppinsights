template<typename T>
int Test(T&& t)
{
    return t();
}

int get(int& v){

  auto x = Test( [&]{ return v; });

  return x;
}


int main(){
    int x = 2;
    return get(x);
}
