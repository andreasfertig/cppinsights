struct demonstrator{
  template <typename return_type = double>
  return_type templated_function() {
    return return_type{};
  }
};

int demonstrate() {
  demonstrator D;
  D.template templated_function<bool>();
  D.template templated_function<float>();
  D.template templated_function();
  return 42;
}
  
