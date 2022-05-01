// cmdlineinsights:-edu-show-noexcept

struct Data
{
  int i;
  double d;
  double func() const noexcept{
	// should't also this show a try ... catch block? 
    return d*i;
  }
};


int func() noexcept {
  return 4*6;
}
