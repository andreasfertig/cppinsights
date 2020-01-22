// cmdline:-std=c++2a
namespace ConceptsDefinition {
    template<typename T>
    concept C = true;

    template<typename T, typename U>
    concept D = true;

    // N4861: [expr.prim.req.simple]
    template<typename T>
    concept SimpleRequirement = requires { T{}; T(); };

    // N4861: [expr.prim.req.compound]
    template<typename T>
    concept CompoundRequirement = requires(T a, T b) {
        { a == b } noexcept; // compound requirement
        a != b;
        { a } -> C;
        { a == b } noexcept -> C;
        { a == b } noexcept -> D<T>;
    };

    // N4861: [expr.prim.req.nested]
    template<typename T>
    concept NestedRequirement = requires(T a, T b) {
        { a == b } noexcept;
        requires C<T>; // nested requirement
    };

    // N4861: [expr.prim.req.type]
    template<typename T>
    concept TypeRequirement = requires(T a, T b) {
        { a == b } noexcept;
        typename T::inner; // type requirement
    };
}

bool areEqual(ConceptsDefinition::CompoundRequirement auto a, ConceptsDefinition::CompoundRequirement auto b) {
  return a == b;
}

int main() {
    ConceptsDefinition::SimpleRequirement auto x = 4;

    return areEqual(2, 3.0);
}
