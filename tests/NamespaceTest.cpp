#include <map>

int main()
{
  std::multimap<int, char> mm{{2019, 'A'}};  
    
  // Here we have std::__map_node_handle_specifics which is a NamedDecl and requires special treament.
  auto jandle = mm.extract(2019);
}

