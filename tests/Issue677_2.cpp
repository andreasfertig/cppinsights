// cmdline:-std=c++20

template<class T>
concept ConceptInNoExcept = true;

template<class T>
void FunWithConceptInNoExcept() noexcept(ConceptInNoExcept<T>)
{}

int main()
{
  FunWithConceptInNoExcept<int>();
}

