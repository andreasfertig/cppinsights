// cmdline:-std=c++20

struct Test
{
   decltype([] { }) a;

   void(*fp)() = (+decltype([] { }){});
};
