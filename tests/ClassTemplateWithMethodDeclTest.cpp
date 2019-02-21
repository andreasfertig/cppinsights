#define INSIGHTS_USE_TEMPLATE

template<typename T>
struct Foo
{
	void bar();
};

template<typename T>
void Foo<T>::bar()
{
}

int main()
{
	Foo<int> f;
	f.bar();
}

