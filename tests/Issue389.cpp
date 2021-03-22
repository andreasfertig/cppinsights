struct Point {
  int x = 0;
  int y = 0;
};


int main() {
    Point pt;
    Point* pPt = &pt;
    const Point* cpPt = &pt;
    Point& lrPt = pt;
    Point&& rrPt = {};
    using T1 = decltype(pt); // Point
    using T2 = decltype(pPt); // Point*
    using T3 = decltype(cpPt); // const Point*
    using T4 = decltype(lrPt); // Point&
    using T5 = decltype(rrPt); // Point&&
}
