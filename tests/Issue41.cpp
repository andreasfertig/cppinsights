#include <vector>
#include <algorithm>
#include <string>

using namespace std;

int main()
{
  vector<string> v = { "aaa", "bbb", "ccc" };
  
  auto it = std::find_if(begin(v), end(v), 
                         [](const auto & str){ return str == "bbb";} );
}
