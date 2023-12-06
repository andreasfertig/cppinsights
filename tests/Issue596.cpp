#include <iostream>
#include <vector>

template<typename T>
class Data {
public:
	Data(const T &t) {
	}
};
Data(const char *) -> Data<std::string> ;
Data(int)->Data<long> ;
int main() {
	Data d1{5};
}

