#include <iostream>
#include <vector>
#include <random>
#include <utility>

namespace malashenko
{
  struct Ellipse
  {
    int a;
    int b;
    int cx;
    int cy;
  };

  struct BoundingBox
  {
    int minX;
    int minY;
    int maxX;
    int maxY;
  };

  BoundingBox getBoundingBox(const std::vector< Ellipse > &ellipses)
  {
    BoundingBox box;

    box.minX = ellipses[0].cx - ellipses[0].a;
    box.maxX = ellipses[0].cx + ellipses[0].a;
    box.minY = ellipses[0].cy - ellipses[0].b;
    box.maxY = ellipses[0].cy + ellipses[0].b;

    for (size_t i = 1; i < ellipses.size(); ++i)
    {
      box.minX = std::min(box.minX, ellipses[i].cx - ellipses[i].a);
      box.maxX = std::max(box.maxX, ellipses[i].cx + ellipses[i].a);
      box.minY = std::min(box.minX, ellipses[i].cy - ellipses[i].b);
      box.maxY = std::max(box.maxX, ellipses[i].cy + ellipses[i].b);
    }

    return box;
  }

  bool isInside(const Ellipse& el, double x, double y)
  {
    double dx = (x - el.cx) / el.a;
    double dy = (y - el.cy) / el.b;

    return dx * dx + dy * dy <= 1.0;
  }

  std::pair< bool, bool > isInsideAnyAll(const std::vector< Ellipse >& ellipses, double x, double y)
  {

    bool isInsideAny = false;
    bool isInsideAll = true;
    for (size_t i = 0; i < ellipses.size(); ++i)
    {
      if(isInside(ellipses[i], x, y))
      {
        isInsideAny = true;
      }
      else
      {
        isInsideAll = false;
      }
    }
    return {isInsideAny, isInsideAll};
  }


  std::pair< size_t, size_t > calc(const std::vector< Ellipse >& ellipses, BoundingBox box, size_t tests, size_t seed)
  {
    std::mt19937 generator(seed);

    std::uniform_real_distribution< double > xDistribution(box.minX, box.maxX);
    std::uniform_real_distribution< double > yDistribution(box.minY, box.maxY);

    size_t insideAny = 0;
    size_t insideAll = 0;


    for (size_t i = 0; i < tests; ++i)
    {
      double x = xDistribution(generator);
      double y = yDistribution(generator);

      std::pair< bool, bool > result = isInsideAnyAll(ellipses, x, y);

      insideAny += result.first;
      insideAll += result.second;
    }
    return {insideAny, insideAll};
  }
}


int main()
{
  std::cout << "malashenko.dmitrii\n";
}
