template<typename T>
constexpr bool is_v = false;

struct out
{
  void operator<<(bool) {}
};

static out cout;

int main() {
   cout << is_v<void(int)>;
}
