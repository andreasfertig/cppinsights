#include <functional>
#include <iostream>
class EventContainer {
    private:
        int val = 1234;
        std::function<void()> something = [=]() {
            std::cout << this->val;
        };

    public:
    EventContainer() : val{1235}{}
};

int main() {
  // get the default constructor generated.
  EventContainer e;
}
