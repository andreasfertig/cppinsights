struct example_struct
{
	example_struct() { throw; }
	~example_struct() { }
};

example_struct& get_example()
{
	static example_struct temp;
	return temp;
}

