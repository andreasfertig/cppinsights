// From: https://en.cppreference.com/w/cpp/language/lambda

auto Fwd = [](int(*fp)(int), auto a){ return fp(a); };

auto C = [](auto a){ return a; };

static_assert( Fwd(C,3) == 3 );

