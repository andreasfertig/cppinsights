#include <numeric>
#include <vector>

int main()
{
  std::vector<int> myVec{1,2,3,4,5};

  auto fak= std::accumulate(myVec.begin(),myVec.end(),1,[](int fir, int sec){ return fir * sec; });
}
