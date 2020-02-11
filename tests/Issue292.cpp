// cmdlineinsights:--show-all-implicit-casts
class H {
  public:
  H(const char*, int, double, double);
  H(const char*, int, double*);
  ~H();
};

auto foo() {
  return H{"hai",4,0};
}
