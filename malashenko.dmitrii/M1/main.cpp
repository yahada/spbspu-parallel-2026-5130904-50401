#include <iostream>
#include <vector>
#include <random>
#include <utility>
#include <cstring>
#include <algorithm>
#include <cstddef>
#include <string>
#include <pthread.h>

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

  BoundingBox getBoundingBox(const std::vector< Ellipse > &ellipses)
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

  std::pair<bool, bool> isInsideAnyAll(const std::vector< Ellipse >& ellipses, double x, double y)
  {

    bool is_inside_any = false;
    bool is_inside_all = true;
    for (size_t i = 0; i < ellipses.size(); ++i)
    {
      if(isInside(ellipses[i], x, y))
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


  std::pair< size_t, size_t > calc(const std::vector< Ellipse >& ellipses, BoundingBox box, size_t tests, size_t seed)
  {
    std::default_random_engine generator(seed);

    std::uniform_real_distribution< double > x_distribution(box.min_x, box.max_x);
    std::uniform_real_distribution< double > y_distribution(box.min_y, box.max_y);

    size_t inside_any = 0;
    size_t inside_all = 0;


    for (size_t i = 0; i < tests; ++i)
    {
      const double x = x_distribution(generator);
      const double y = y_distribution(generator);

      const std::pair< bool, bool > result = isInsideAnyAll(ellipses, x, y);

      inside_any += result.first;
      inside_all += result.second;
    }
    return {inside_any, inside_all};
  }

  std::pair< double, double > getAreaAnyAll(BoundingBox box, size_t tests, size_t inside_all, size_t inside_any)
  {
    const double box_area = (box.max_x - box.min_x) * (box.max_y - box.min_y);
    const double area_of_any = box_area * (static_cast< double >(inside_any) / tests);
    const double area_of_all = box_area * (static_cast< double >(inside_all) / tests);

    return {area_of_any, area_of_all};
  }

  struct ThreadData {
    const std::vector< Ellipse >* ellipses;
    BoundingBox box;
    size_t tests;
    size_t seed;

    size_t inside_any;
    size_t inside_all;
  };

  void* threadFunction(void* arg)
  {
    ThreadData* const data = static_cast< ThreadData* >(arg);

    const std::pair<size_t, size_t> result = calc(*data->ellipses, data->box, data->tests, data->seed);

    data->inside_any = result.first;
    data->inside_all = result.second;

    return nullptr;
  }

  std::pair< size_t, size_t > calcParallel(const std::vector<Ellipse> &ellipses,  BoundingBox box,
                                            size_t threads, size_t tests, size_t seed)
  {
    std::vector< pthread_t > thread_ids(threads);
    std::vector< ThreadData > thread_data(threads);

    const size_t tests_per_thread = tests / threads;
    const size_t remainder = tests % threads;

    for (size_t i = 0; i < threads; ++i)
    {
      thread_data[i].ellipses = &ellipses;
      thread_data[i].box = box;

      thread_data[i].tests = tests_per_thread;

      if (i == threads - 1)
      {
        thread_data[i].tests += remainder;
      }

      thread_data[i].seed = seed + i;

      thread_data[i].inside_any = 0;
      thread_data[i].inside_all = 0;

      const int err = pthread_create(&thread_ids[i], nullptr, threadFunction, &thread_data[i]);

      if (err != 0)
      {
        std::cerr << "pthread_create: " << strerror(err) << '\n';
        return {0, 0};
      }
    }

    size_t total_inside_any = 0;
    size_t total_inside_all = 0;

    for (size_t i = 0; i < threads; ++i)
    {
      const int err = pthread_join(thread_ids[i], nullptr);

      if (err != 0)
      {
        std::cerr << "pthread_join: " << strerror(err) << '\n';

        return {0, 0};
      }

      total_inside_any += thread_data[i].inside_any;
      total_inside_all += thread_data[i].inside_all;
    }

    return {total_inside_any, total_inside_all};
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

  int a = 0, b = 0, cx = 0, cy = 0;

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

  const std::pair< size_t, size_t > result = malashenko::calcParallel(ellipses, box, threads, tests, seed);

  const std::pair< double, double > areas = malashenko::getAreaAnyAll(box, tests, result.first, result.second);

  std::cout << areas.second << ' ' << areas.first << '\n';

  return 0;
}
