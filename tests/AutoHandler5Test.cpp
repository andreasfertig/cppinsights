int foo(int a,int b){ return a+b; }

int main(){
  if(true) {
    int (*add)(int,int)= foo;
    // auto within a function pointer
    auto add1= foo;         
  }
}

