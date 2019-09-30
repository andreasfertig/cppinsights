// cmdline:-std=c++2a
void f() { auto lambda = [](auto init, auto container) { for(auto i=init; auto test : container) { } }; }
