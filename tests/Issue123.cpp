#define INSIGHTS_USE_TEMPLATE

template<typename T>
struct Person
{
	int age, heightInInches;
	T distanceTraveled = T(); //default value
	float hairLength, GPA;
	unsigned int SATScore;

	T run(T speed, bool startWithLeftFoot);
};

template<typename T>
T Person<T>::run(T speed, bool left )
{
	return speed + (left ? 1 : 0);
}

template class Person<int>;

int main()
{
	Person<int> f;
	auto x = f.run(2, true);
}
