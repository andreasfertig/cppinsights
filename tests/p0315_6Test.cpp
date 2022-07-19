// cmdline:-std=c++20

decltype((+decltype([] {}){})) a = (+decltype([] {}){});

void (*fp)() = [] {};
