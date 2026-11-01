#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP
#include <vector>

namespace malashenko
{
  struct Circle
  {
    int radius;
    int x;
    int y;
  };

  struct BoundingBox
  {
    int min_x;
    int min_y;
    int max_x;
    int max_y;
  };

  BoundingBox getBoundingBox(const Circle& circle);

  BoundingBox getBoundingBox(const std::vector< Circle >& circles);

}

#endif
