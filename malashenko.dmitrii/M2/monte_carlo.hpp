#ifndef MONTE_CARLO_HPP
#define MONTE_CARLO_HPP

#include <cstddef>
#include <vector>
#include "geometry.hpp"

namespace malashenko
{

  struct CalculationParams
  {
    const std::vector< Circle > &circles;
    BoundingBox box;
    size_t threads;
    size_t tries;
    size_t seed;
  };

  double calculateArea(CalculationParams &params);
  size_t calcParallel(const CalculationParams &params);
  double getArea(const BoundingBox &box, size_t tries, size_t inside_any);
  size_t calc(const CalculationParams &params);

}

#endif
