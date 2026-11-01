#include "geometry.hpp"

#include <algorithm>
#include <stdexcept>

malashenko::BoundingBox malashenko::getBoundingBox(const Circle &circle)
{
  if (circle.radius <= 0)
  {
    throw std::invalid_argument("Invalid circle radius");
  }

  return {
      (circle.x) - circle.radius, (circle.y) - circle.radius, (circle.x) + circle.radius, (circle.y) + circle.radius};
}

malashenko::BoundingBox malashenko::getBoundingBox(const std::vector< Circle > &circles)
{
  if (circles.empty())
  {
    throw std::invalid_argument("Empty circle set");
  }

  BoundingBox result = getBoundingBox(circles[0]);

  for (size_t i = 1; i < circles.size(); ++i)
  {
    const BoundingBox current = getBoundingBox(circles[i]);

    result.min_x = std::min(result.min_x, current.min_x);
    result.min_y = std::min(result.min_y, current.min_y);

    result.max_x = std::max(result.max_x, current.max_x);
    result.max_y = std::max(result.max_y, current.max_y);
  }

  return result;
}

