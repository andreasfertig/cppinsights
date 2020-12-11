template<long Len = 128, long BlockSize = 128, class T = wchar_t>
class String {
public:
  bool Format(const wchar_t FormatStr[], ...) { return true; }
};

void FTest() {
  double d{};

  String s;
  s.Format(L"%i", d);
}

