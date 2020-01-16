#include <iostream>
#include <utility>

namespace expressions {

template<class Expr1,class Expr2>
auto operator||(Expr1&& ex1,Expr2&& ex2)
{

  return [ex1=std::move(ex1),ex2=std::move(ex2)](auto&& val) 
  {
    return (ex1(std::forward<decltype(val)>(val)) || ex2(std::forward<decltype(val)>(val)));
  };
}


template<class Expr1,class Expr2>
auto operator&&(Expr1&& ex1,Expr2&& ex2)
{
  return [ex1=std::move(ex1),ex2=std::move(ex2)](auto val) 
  {
    
    return (ex1(std::forward<decltype(val)>(val)) && ex2(std::forward<decltype(val)>(val)));
  };
}

  auto Gt_10 = [](int val)
  {
    return val > 10;     
  };
  
  auto Lt_20 = [](int val)
  {
    return val < 20;
  };
}

int main()
{
  
  auto f = expressions::Gt_10 && expressions::Lt_20;
  std::cout << "Ok?" <<f(15) << '\n';
  
  return 0;
  
}
