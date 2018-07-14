#include <stdexcept>

int main()
{
    try {
      int x = 2;
      throw std::logic_error("Error");
    }
    catch (std::runtime_error& e){
      return -1;
    }
    catch (std::logic_error& e){
      return -1;
    }

    
    return 0;
}
