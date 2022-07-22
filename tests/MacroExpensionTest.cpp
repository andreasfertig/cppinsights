#define SOME_NS(name)                                                                                                  \
    namespace name {                                                                                                   \
        int x;                                                                                                         \
    }

SOME_NS(Test)

#define SOME_ENUM(name)                                                                                                \
    enum name {                                                                                                        \
        good,                                                                                                          \
        bad,                                                                                                           \
    }

SOME_ENUM(eTest);

#define SOME_STATIC_ASSERT(cond) static_assert(cond)

SOME_STATIC_ASSERT(true);

#define SOME_VAR_TEMPLATE_DECL(V, name)                                                                                \
    template<class T>                                                                                                  \
    constexpr T pi    = T(3.1415926535897932385L);                                                                     \
    auto        name  = pi<V>



SOME_VAR_TEMPLATE_DECL(int, i);

