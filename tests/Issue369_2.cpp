struct foo{foo();};

const void withNoexcept(){
  	static foo value = []() noexcept {return foo();}(); 
}

const void withNoexceptFalse(){
  	static foo value = []() noexcept(false) {return foo();}(); 
}

const void withNoexceptTrue(){
  	static foo value = []() noexcept(true) {return foo();}(); 
}


struct buh { };

const void withNoexceptNoexceptCtor(){
  	static buh value = []() { return buh();}(); 
}


foo func()
{
    return {};
}


void viaFunction() {
    static foo  f = func();
}
