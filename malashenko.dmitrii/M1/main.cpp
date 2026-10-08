#include <algorithm>
#include <cstddef>
#include <iostream>
#include <future>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace malashenko
{
  struct Ellipse
  {
    int a, b, cx, cy;
  };

  struct BoundingBox
  {
    int min_x, min_y, max_x, max_y;
  };

  struct CalculationParams {
    const std::vector< Ellipse >& ellipses;
    BoundingBox box;
    size_t threads, tests, seed;
  };

  BoundingBox getBoundingBox(const std::vector< Ellipse >& ellipses)
  {
    BoundingBox box;

    box.min_x = ellipses[0].cx - ellipses[0].a;
    box.max_x = ellipses[0].cx + ellipses[0].a;
    box.min_y = ellipses[0].cy - ellipses[0].b;
    box.max_y = ellipses[0].cy + ellipses[0].b;

    for (size_t i = 1; i < ellipses.size(); ++i)
    {
      box.min_x = std::min(box.min_x, ellipses[i].cx - ellipses[i].a);
      box.max_x = std::max(box.max_x, ellipses[i].cx + ellipses[i].a);
      box.min_y = std::min(box.min_y, ellipses[i].cy - ellipses[i].b);
      box.max_y = std::max(box.max_y, ellipses[i].cy + ellipses[i].b);
    }

    return box;
  }

  bool isInside(const Ellipse& el, double x, double y)
  {
    const double dx = (x - el.cx) / el.a;
    const double dy = (y - el.cy) / el.b;

    return dx * dx + dy * dy <= 1.0;
  }

  std::pair< bool, bool > isInsideAnyAll(const std::vector< Ellipse >& ellipses, double x, double y)
  {
    bool is_inside_any = false;
    bool is_inside_all = true;
    for (size_t i = 0; i < ellipses.size(); ++i)
    {
      if (isInside(ellipses[i], x, y))
      {
        is_inside_any = true;
      }
      else
      {
        is_inside_all = false;
      }
    }
    return {is_inside_any, is_inside_all};
  }

  std::pair< size_t, size_t > calc(const CalculationParams& params)
  {
    std::default_random_engine generator(params.seed);

    std::uniform_real_distribution< double > x_distribution(params.box.min_x, params.box.max_x);
    std::uniform_real_distribution< double > y_distribution(params.box.min_y, params.box.max_y);

    size_t inside_any = 0;
    size_t inside_all = 0;

    for (size_t i = 0; i < params.tests; ++i)
    {
      const double x = x_distribution(generator);
      const double y = y_distribution(generator);

      const std::pair< bool, bool > result = isInsideAnyAll(params.ellipses, x, y);

      inside_any += result.first;
      inside_all += result.second;
    }
    return {inside_any, inside_all};
  }

  std::pair< double, double > getAreaAnyAll(BoundingBox box, size_t tests, size_t inside_any, size_t inside_all)
  {
    const double box_area = (box.max_x - box.min_x) * (box.max_y - box.min_y);
    const double area_of_any = box_area * (static_cast< double >(inside_any) / tests);
    const double area_of_all = box_area * (static_cast< double >(inside_all) / tests);

    return {area_of_any, area_of_all};
  }

  std::pair< size_t, size_t > calcParallel(const CalculationParams& params)
  {
    std::vector< std::future< std::pair< size_t, size_t > > > futures;

    const size_t tests_per_thread = params.tests / params.threads;
    const size_t remainder = params.tests % params.threads;

    for (size_t i = 0; i < params.threads; ++i)
    {
      const size_t thread_tests = tests_per_thread + (i == params.threads - 1 ? remainder : 0);

      CalculationParams thread_params = params;
      thread_params.tests = thread_tests;
      thread_params.seed += i;

      futures.emplace_back(std::async(std::launch::async, calc, thread_params));
    }

    size_t inside_any = 0;
    size_t inside_all = 0;

    for (auto& future : futures)
    {
      const std::pair< size_t, size_t > result = future.get();

      inside_any += result.first;
      inside_all += result.second;
    }

    return {inside_any, inside_all};
  }
}

int main(int argc, char* argv[])
{
  if (argc != 3 && argc != 4)
  {
    std::cerr << "Usage: ./lab threads tries [seed]\n";
    return 1;
  }

  long long threads_input = 0;
  long long tests_input = 0;
  long long seed_input = 0;

  try
  {
    threads_input = std::stoll(argv[1]);
    tests_input = std::stoll(argv[2]);

    if (argc == 4)
    {
      seed_input = std::stoll(argv[3]);
    }
  }
  catch (...)
  {
    std::cerr << "Invalid command line arguments\n";
    return 1;
  }

  if (threads_input < 0 || (argc == 4 && seed_input < 0))
  {
    std::cerr << "Threads and seed cannot be negative\n";
    return 1;
  }

  if (tests_input <= 0)
  {
    std::cerr << "Number of tests must be positive\n";
    return 1;
  }

  threads_input = threads_input == 0 ? 1 : threads_input;

  const size_t threads = static_cast< size_t >(threads_input);
  const size_t tests = static_cast< size_t >(tests_input);
  const size_t seed = static_cast< size_t >(seed_input);

  std::vector< malashenko::Ellipse > ellipses;

  int a = 0;
  int b = 0;
  int cx = 0;
  int cy = 0;

  while (std::cin >> a >> b >> cx >> cy)
  {
    if (a <= 0 || b < 0)
    {
      std::cerr << "Invalid figure parameters\n";
      return 1;
    }

    if (b == 0)
    {
      ellipses.push_back({a, a, cx, cy});
    }
    else
    {
      ellipses.push_back({a, b, cx, cy});
    }
  }

  if (!std::cin.eof())
  {
    std::cerr << "Failed to parse figure\n";
    return 1;
  }

  if (ellipses.empty())
  {
    std::cerr << "No figures provided\n";
    return 1;
  }

  const malashenko::BoundingBox box = malashenko::getBoundingBox(ellipses);

  const malashenko::CalculationParams params{ellipses, box, threads, tests, seed};
  const std::pair< size_t, size_t > result = malashenko::calcParallel(params);

  const std::pair< double, double > areas = malashenko::getAreaAnyAll(box, tests, result.first, result.second);

  std::cout << areas.first << ' ' << areas.second << '\n';

  return 0;
}
