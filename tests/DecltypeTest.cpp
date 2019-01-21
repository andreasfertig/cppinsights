int main()
{
  const char s[] = "Hello";
  
  const char* const cs = s;
 
  decltype(cs) cas = cs;
}
