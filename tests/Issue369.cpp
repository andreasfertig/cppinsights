struct foo{foo();};

const foo& create(){
  	static const foo value = foo(); // exception handling missing
  	return value;
}

