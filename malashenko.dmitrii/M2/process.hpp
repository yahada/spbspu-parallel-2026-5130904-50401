#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <vector>
#include <sys/types.h>
#include <map>
#include <future>
#include "geometry.hpp"

namespace malashenko
{
  using circles_t = const std::vector< Circle >&;

  enum class CalcStatus
  {
    in_progress,
    done,
    failed
  };

  struct CalculationResult
  {
    double milliseconds;
    double area;
  };

  struct Process
  {
    pid_t pid;
    int command_fd;
    int response_fd;
    bool alive;
  };

  struct CalcResponse
  {
    size_t id;
    CalcStatus status;
    double milliseconds;
    double area;
  };

  enum class RequestType
  {
    start,
    status,
    wait
  };

  struct Request
  {
    RequestType type;
    size_t id;
    size_t threads;
    size_t tries;
    size_t circles_count;
  };

  using PendingCalculations = std::map< size_t, std::future< CalcResponse > >;

  using FinishedCalculations = std::map< size_t, CalcResponse >;

  bool writeAll(int fd, const void* data, size_t size);

  bool readAll(int fd, void* data, size_t size);

  CalcResponse calculateInWorker(std::vector< Circle > circles, size_t id, size_t threads, size_t tries, size_t seed);

  CalcResponse checkCalculation(
      size_t id, bool should_wait, PendingCalculations& pending, FinishedCalculations& finished);

  int workerLoop(int command_fd, int response_fd, size_t seed);

  Process spawnProcess(size_t seed, const std::vector< Process >& existing_processes);

  CalcResponse startCalc(Process& process, size_t id, circles_t circles, size_t threads, size_t tries);

  CalcResponse queryCalc(Process& process, size_t id, bool wait);

  void closeProcess(Process& process);
}

#endif
