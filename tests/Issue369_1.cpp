struct foo{foo();};

const foo& create(){
  	static foo value = [](){return foo();}(); // exception handling missing
  	return value;
}

