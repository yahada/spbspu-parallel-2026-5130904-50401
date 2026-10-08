#include <iostream>
#include <vector>
#include <random>
#include <utility>
#include <pthread.h>
#include <cstring>
#include <algorithm>
#include <cstddef>

namespace malashenko
{
  struct Ellipse
  {
    int a, b, cx, cy;
  };

  struct BoundingBox
  {
    int minX, minY, maxX, maxY;
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
      box.minY = std::min(box.minY, ellipses[i].cy - ellipses[i].b);
      box.maxY = std::max(box.maxY, ellipses[i].cy + ellipses[i].b);
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

  std::pair< double, double > getAreaAnyAll(BoundingBox box, size_t tests, size_t insideAll, size_t insideAny)
  {
    double boxArea = (box.maxX - box.minX) * (box.maxY - box.minY);
    double areaOfAny = boxArea * (static_cast< double >(insideAny) / tests);
    double areaOfAll = boxArea * (static_cast< double >(insideAll) / tests);

    return {areaOfAny, areaOfAll};
  }

  struct ThreadData {
    const std::vector< Ellipse >* ellipses;
    BoundingBox box;
    size_t tests;
    size_t seed;

    size_t insideAny;
    size_t insideAll;
  };

  void* threadFunction(void* arg)
  {
    ThreadData* data = static_cast< ThreadData* >(arg);

    auto result = calc(*data->ellipses, data->box, data->tests, data->seed);

    data->insideAny = result.first;
    data->insideAll = result.second;

    return nullptr;
  }

  std::pair< size_t, size_t > calcParallel(const std::vector<Ellipse> &ellipses,  BoundingBox box,
                                            size_t threads, size_t tests, size_t seed)
  {
    std::vector< pthread_t > threadIds(threads);
    std::vector< ThreadData > threadData(threads);

    size_t testsPerThread = tests / threads;
    size_t remainder = tests % threads;

    for (size_t i = 0; i < threads; ++i)
    {
      threadData[i].ellipses = &ellipses;
      threadData[i].box = box;

      threadData[i].tests = testsPerThread;

      if (i == threads - 1)
      {
        threadData[i].tests += remainder;
      }

      threadData[i].seed = seed + i;

      threadData[i].insideAny = 0;
      threadData[i].insideAll = 0;

      int err = pthread_create(&threadIds[i], nullptr, threadFunction, &threadData[i]);

      if (err != 0)
      {
        std::cerr << "pthread_create: " << strerror(err) << '\n';
        return {0, 0};
      }
    }

    size_t totalInsideAny = 0;
    size_t totalInsideAll = 0;

    for (size_t i = 0; i < threads; ++i)
    {
      int err = pthread_join(threadIds[i], nullptr);

      if (err != 0)
      {
        std::cerr << "pthread_join: " << strerror(err) << '\n';

        return {0, 0};
      }

      totalInsideAny += threadData[i].insideAny;
      totalInsideAll += threadData[i].insideAll;
    }

    return {totalInsideAny, totalInsideAll};
  }
}


int main(int argc, char* argv[])
{
  if (argc != 3 && argc != 4)
  {
    std::cerr << "Usage: ./lab threads tries [seed]\n";
    return 1;
  }

  size_t threadsInput;
  size_t testsInput;
  size_t seedInput = 0;

  try
  {
    threadsInput = std::stoll(argv[1]);
    testsInput = std::stoll(argv[2]);

    if (argc == 4)
    {
      seedInput = std::stoll(argv[3]);
    }
  }
  catch (const std::exception&)
  {
    std::cerr << "Invalid command line arguments\n";
    return 1;
  }

  if (threadsInput < 0 || (argc == 4 && seedInput < 0))
  {
    std::cerr << "Threads and seed cannot be negative\n";
    return 1;
  }

  if (testsInput <= 0)
  {
    std::cerr << "Number of tests must be positive\n";
    return 1;
  }

  if (threadsInput == 0)
  {
    std::cerr << "Number of threads must be positive\n";
    return 1;
  }

  size_t threads = static_cast< size_t >(threadsInput);
  size_t tests = static_cast< size_t >(testsInput);
  size_t seed = static_cast< size_t >(seedInput);

  std::vector<malashenko::Ellipse> ellipses;

  int a, b, cx, cy;

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

  malashenko::BoundingBox box = malashenko::getBoundingBox(ellipses);

  auto result = malashenko::calcParallel(ellipses, box, threads, tests, seed);

  auto areas = malashenko::getAreaAnyAll(box, tests, result.first, result.second);

  std::cout << areas.second << ' ' << areas.first << '\n';

  return 0;
}
