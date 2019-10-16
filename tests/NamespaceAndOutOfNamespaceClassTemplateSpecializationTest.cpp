namespace NamespaceAndOutOfNamespaceClassTemplateSpecializationTest
{
    template<typename T>
    struct Base {};
}

// Declare the specialization outside of the namespace with a namespace specifier
template<> 
struct NamespaceAndOutOfNamespaceClassTemplateSpecializationTest::Base<int> {};
