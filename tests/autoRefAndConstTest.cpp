#include <vector>

const std::vector<int>& getTheData() {
  static std::vector<int> theData;
  return theData;
}

int main()
{
  auto& x = getTheData();
}
