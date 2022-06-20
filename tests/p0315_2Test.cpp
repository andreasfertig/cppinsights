// cmdline:-std=c++20

#include <array>

static std::array<decltype([] { }), 2> ff();

template<typename T, int SIZE>
class Container {
};

static Container<decltype([] { }), 2> c();

static decltype([] { })& f();

static void h(decltype([] { })*) { }

static void w(decltype([] { })*, decltype([] { return 2; })&) { }

// This is invalid syntax!
//static decltype([] { }) x()[2];

