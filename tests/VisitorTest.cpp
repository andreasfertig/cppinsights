template<class... Ts> struct visitor: Ts... { using Ts::operator()...; };
template<class... Ts> visitor(Ts...) -> visitor<Ts...>;


int main()
{
    auto my_visitor = visitor{
      [&](int value) { /* ... */ },
      [&](const char* value) { /* ... */ },
    };


    my_visitor("hello");
    my_visitor(2);
}
