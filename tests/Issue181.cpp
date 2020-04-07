int main()
{
	int x = 1, y = 2;
    
	auto f = [&](auto self) {
		(void)x; (void)y; // make 'em used!
		auto& [a, b] = *self; // a == 1, b == 2. Depends on the previous line.
		return b - a;
	};
	return f(&f); // returns 1
}
