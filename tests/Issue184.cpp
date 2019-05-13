#include <string>
#include <unordered_set>

class Customer{
std::string name;
public:
Customer(const std::string &n) : name(n){}
std::string getName()const{return name;}
};

struct CustomerEq{
bool operator()(const Customer&c0,const Customer&c1)const{
return c0.getName() == c1.getName();
}
};

struct CustomerHash{
std::size_t operator()(const Customer& c) const{
return std::hash<std::string>()(c.getName());
}
};

template<typename ... Bases>
struct ManyParentsWithOperator : Bases...{
using Bases::operator()...;//C++17
};

int main()
{
    std::unordered_set<Customer, CustomerHash, CustomerEq> set1;
    std::unordered_set<Customer, ManyParentsWithOperator<CustomerHash, CustomerEq>, ManyParentsWithOperator<CustomerHash, CustomerEq>> set2;
    std::unordered_set<Customer, ManyParentsWithOperator<CustomerEq, CustomerHash>, ManyParentsWithOperator<CustomerEq, CustomerHash>> set3;

    set1.emplace("Test");
    set2.emplace("Test");
    set3.emplace("Test");
    return 0;
}
