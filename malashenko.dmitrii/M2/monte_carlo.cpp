#include "monte_carlo.hpp"

#include <algorithm>
#include <future>
#include <random>
#include <stdexcept>
#include <vector>

size_t malashenko::calc(const CalculationParams &params)
{
  using Engine = std::default_random_engine;

  Engine generator(params.seed);

  const double engine_min = static_cast< double >(Engine::min());
  const double engine_range = static_cast< double >(Engine::max() - Engine::min()) + 1.0;
  const double inv_range = 1.0 / engine_range;

  const double box_min_x = params.box.min_x;
  const double box_min_y = params.box.min_y;
  const double box_range_x = static_cast< double >(params.box.max_x) - params.box.min_x;
  const double box_range_y = static_cast< double >(params.box.max_y) - params.box.min_y;

  size_t inside_any = 0;

  for (size_t i = 0; i < params.tries; ++i)
  {
    const double u1 = (static_cast< double >(generator()) - engine_min) * inv_range;
    const double u2 = (static_cast< double >(generator()) - engine_min) * inv_range;

    const double x = box_min_x + u1 * box_range_x;
    const double y = box_min_y + u2 * box_range_y;

    bool inside = false;
    for (const Circle &circle : params.circles)
    {
      const double dx = x - circle.x;
      const double dy = y - circle.y;
      const double radius = static_cast< double >(circle.radius);
      if (dx * dx + dy * dy <= radius * radius)
      {
        inside = true;
        break;
      }
    }
    inside_any += inside;
  }

  return inside_any;
}

double malashenko::getArea(const BoundingBox &box, size_t tries, size_t inside_any)
{
  const double width = static_cast< double >(box.max_x) - box.min_x;

  const double height = static_cast< double >(box.max_y) - box.min_y;

  const double box_area = width * height;

  return box_area * (static_cast< double >(inside_any) / tries);
}

size_t malashenko::calcParallel(const CalculationParams &params)
{
  if (params.threads == 1)
  {
    return calc(params);
  }

  const size_t actual_threads = std::min(params.threads, params.tries);

  const size_t tries_per_thread = params.tries / actual_threads;

  const size_t remainder = params.tries % actual_threads;

  std::vector< std::future< size_t > > futures;

  futures.reserve(actual_threads);

  for (size_t i = 0; i < actual_threads; ++i)
  {
    const size_t thread_tries = tries_per_thread + (i == actual_threads - 1 ? remainder : 0);

    CalculationParams thread_params = params;

    thread_params.tries = thread_tries;
    thread_params.seed += i;

    futures.emplace_back(std::async(std::launch::async, calc, thread_params));
  }

  size_t inside_any = 0;

  for (auto &future : futures)
  {
    inside_any += future.get();
  }

  return inside_any;
}

double malashenko::calculateArea(CalculationParams &params)
{
  if (params.circles.empty() || params.threads == 0 || params.tries == 0)
  {
    throw std::invalid_argument("Invalid calculation parameters");
  }

  const BoundingBox box = getBoundingBox(params.circles);

  params.box = box;

  const size_t inside_any = calcParallel(params);

  return getArea(box, params.tries, inside_any);
}
