#include <iostream>
namespace malashenko
{
  struct Ellipse
  {
    int a;
    int b;
    int cx;
    int cy;
  };

  bool isInside(int a, int b, int cx, int cy, double x, double y)
  {
    double dx = (x - cx) / a;
    double dy = (y - cy) / b;

    return dx * dx + dy * dy <= 1.0;
  }
}


int main()
{
  std::cout << "malashenko.dmitrii\n";
}
