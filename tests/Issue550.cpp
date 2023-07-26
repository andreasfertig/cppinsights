// cmdline:-std=c++20

//this is correct
auto temp = []() {};

//wrong
auto temp2 = []<typename>() {};

int main() 
{ 
  temp.operator()();
  
  temp2.operator()<int>();

  //wrong
  int v = 1;
  auto temp3 = [v]() {};
  temp3.operator()();
}
