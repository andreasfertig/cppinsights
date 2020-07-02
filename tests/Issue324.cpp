struct Str {
  Str(const char* string) : string(string) {}
  
  operator const char*() const { 
    return string; 
  }
  
  const char* string;
};

Str globalString = "test";


const char* getString(bool empty) {
  return empty ? "" : globalString;
}
