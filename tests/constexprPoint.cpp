 class Point {
 public:
   constexpr Point(double x, double y) noexcept : mX(x), mY(y) {}

   constexpr double GetX() const noexcept { return mX; } 
   constexpr double GetY() const noexcept { return mY; }

   constexpr void SetX(double x) noexcept { mX = x; }
   constexpr void SetY(double y) noexcept { mY = y; }
 private:
   double mX, mY;
 };



int main()
{
    constexpr Point p{2.0, 3.0};

    constexpr double x = /*p.GetX()*/ 2.0f;
  
    return p.GetX();
}

