void Foo() noexcept(false)
{
}

// ConditionalOperator
void Bar() noexcept(true ? false : true){}

// BinaryOperator
void BBar() noexcept(1==1){}

void FoBar() noexcept{}
