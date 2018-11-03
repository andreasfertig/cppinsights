struct Foo {};

// invalid use of decltype() leads to locEnd.isInvalid()
decltype(auto) (Foo::*bar)();

