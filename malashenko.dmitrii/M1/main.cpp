#include <iostream>
#include <vector>

namespace malashenko
{
  struct Ellipse
  {
    int a;
    int b;
    int cx;
    int cy;
  };

  bool isInside(const Ellipse& el, double x, double y)
  {
    double dx = (x - el.cx) / el.a;
    double dy = (y - el.cy) / el.b;

    return dx * dx + dy * dy <= 1.0;
  }

  bool isInsideAll(double x, double y, const std::vector< Ellipse >& ellipses)
  {
    for (size_t i = 0; i < ellipses.size(); ++i)
    {
      if(!isInside(ellipses[i], x, y))
      {
        return false;
      }
    }
    return true;
  }


}


int main()
{
  std::cout << "malashenko.dmitrii\n";
}
