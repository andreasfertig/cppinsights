struct C { 
  int &i; 
  C() = default;
  C(C const&) = default;
};

int main() {
    int i;
	C c{i};
  
}

