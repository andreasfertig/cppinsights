void foo() {
[]() {
 // (void)X{1, 2, 3}; // => X(std::initializer_list<int>{1, 2, 3})
 // (void)X{nullptr, 0}; // => X(nullptr, 0)
//  takes_y({1, 2}); // => Y _tmp = {1, 2}; takes_y(_tmp);
}();
}


