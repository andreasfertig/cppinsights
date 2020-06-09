// The problem with an anonymous struct is, that the resulting VarDecl rewrites the same source location.
// In a namespace, where the namespace itself is matched, it works.
namespace works {
struct {
    int c;
} RR;
}

// However, the same code with the TU as root does not
struct {
    int c;
    int c1;
    int c2;
    int c3;
    int c4;
    int c6;
} RR;

// However, the same code with the TU as root does not
struct {
    int c;
} RR2;

