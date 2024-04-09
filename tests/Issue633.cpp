// cmdline:-std=c++20

#include <iostream>
#include <map>

int main(){
    std::map<int,int> a={{1,3},{4,5}};
    for(auto &[k,v] : a){
       [k]{return k;}();
    }
}
