// std::function mock up
template<typename ReturnValue, typename... Args>
class function
{
public:
    function() = default;

    template<typename T>
    function(T&& f)
    {}
};

// part of #205
class EventContainer {
    private:
        int val = 1234;
        function<void()> something = [=]() {
            this->val;
        };
};

int main() {
  // get the default constructor generated.
  EventContainer e;
}
