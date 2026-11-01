#include "process.hpp"

#include <cerrno>
#include <chrono>
#include <future>
#include <map>
#include <system_error>
#include <utility>
#include <vector>
#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <cstddef>

#include "monte_carlo.hpp"

bool malashenko::writeAll(int fd, const void* data, size_t size)
{
  const char* const buffer = static_cast< const char* >(data);
  size_t written = 0;

  while (written < size)
  {
    const ssize_t result = write(fd, buffer + written, size - written);

    if (result < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }

      return false;
    }

    if (result == 0)
    {
      return false;
    }

    written += static_cast< size_t >(result);
  }

  return true;
}

bool malashenko::readAll(int fd, void* data, size_t size)
{
  char* const buffer = static_cast< char* >(data);
  size_t received = 0;

  while (received < size)
  {
    const ssize_t result = read(fd, buffer + received, size - received);

    if (result < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }

      return false;
    }

    if (result == 0)
    {
      return false;
    }

    received += static_cast< size_t >(result);
  }

  return true;
}

malashenko::CalcResponse malashenko::calculateInWorker(
    std::vector< Circle > circles, size_t id, size_t threads, size_t tries, size_t seed)
{
  const auto start = std::chrono::steady_clock::now();

  CalculationParams params = {circles, BoundingBox(), threads, tries, seed};
  const double area = calculateArea(params);

  const auto finish = std::chrono::steady_clock::now();

  const double milliseconds = std::chrono::duration< double, std::milli >(finish - start).count();

  return {id, CalcStatus::done, milliseconds, area};
}

malashenko::CalcResponse malashenko::checkCalculation(
    size_t id, bool should_wait, PendingCalculations& pending, FinishedCalculations& finished)
{
  const auto finished_it = finished.find(id);

  if (finished_it != finished.end())
  {
    return finished_it->second;
  }

  const auto pending_it = pending.find(id);

  if (pending_it == pending.end())
  {
    return {id, CalcStatus::failed, 0.0, 0.0};
  }

  if (should_wait)
  {
    pending_it->second.wait();
  }
  else
  {
    if (pending_it->second.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
      return {id, CalcStatus::in_progress, 0.0, 0.0};
    }
  }

  CalcResponse response{};

  try
  {
    response = pending_it->second.get();
  }
  catch (...)
  {
    response = {id, CalcStatus::failed, 0.0, 0.0};
  }

  pending.erase(pending_it);
  finished.emplace(id, response);

  return response;
}

int malashenko::workerLoop(int command_fd, int response_fd, size_t seed)
{
  PendingCalculations pending;
  FinishedCalculations finished;

  Request request{};

  while (readAll(command_fd, &request, sizeof(request)))
  {
    CalcResponse response{request.id, CalcStatus::failed, 0.0, 0.0};

    switch (request.type)
    {
    case RequestType::start:
    {
      std::vector< Circle > circles(request.circles_count);

      if (request.circles_count == 0 || !readAll(command_fd, circles.data(), circles.size() * sizeof(Circle)))
      {
        return 1;
      }

      size_t rid = request.id;
      if (request.threads == 0 || request.tries == 0 || pending.count(rid) != 0 || finished.count(rid) != 0)
      {
        break;
      }

      try
      {
        pending.emplace(rid,
            std::async(std::launch::async, calculateInWorker, std::move(circles), rid, request.threads, request.tries,
                seed + rid));

        response.status = CalcStatus::in_progress;
      }
      catch (...)
      {
        response.status = CalcStatus::failed;
      }

      break;
    }

    case RequestType::status:
    {
      response = checkCalculation(request.id, false, pending, finished);

      break;
    }

    case RequestType::wait:
    {
      response = checkCalculation(request.id, true, pending, finished);

      break;
    }

    default:
      response.status = CalcStatus::failed;
      break;
    }

    if (!writeAll(response_fd, &response, sizeof(response)))
    {
      break;
    }
  }

  return 0;
}

malashenko::Process malashenko::spawnProcess(size_t seed, const std::vector< Process >& existing_processes)
{
  int commands[2] = {-1, -1};
  int responses[2] = {-1, -1};

  std::cout.flush();
  std::cerr.flush();

  if (pipe(commands) == -1)
  {
    throw std::system_error(errno, std::generic_category(), "pipe");
  }

  if (pipe(responses) == -1)
  {
    const int error = errno;

    close(commands[0]);
    close(commands[1]);

    throw std::system_error(error, std::generic_category(), "pipe");
  }

  const pid_t pid = fork();

  if (pid == -1)
  {
    const int error = errno;

    close(commands[0]);
    close(commands[1]);
    close(responses[0]);
    close(responses[1]);

    throw std::system_error(error, std::generic_category(), "fork");
  }

  if (pid == 0)
  {
    close(commands[1]);
    close(responses[0]);

    for (const Process& process : existing_processes)
    {
      if (process.command_fd >= 0)
      {
        close(process.command_fd);
      }

      if (process.response_fd >= 0)
      {
        close(process.response_fd);
      }
    }

    const int result = workerLoop(commands[0], responses[1], seed);

    close(commands[0]);
    close(responses[1]);

    std::_Exit(result);
  }

  close(commands[0]);
  close(responses[1]);

  return {pid, commands[1], responses[0], true};
}

malashenko::CalcResponse malashenko::startCalc(
    Process& process, size_t id, circles_t circles, size_t threads, size_t tries)
{
  if (!process.alive || circles.empty() || threads == 0 || tries == 0)
  {
    throw std::invalid_argument("Invalid calculation");
  }

  if (circles.size() > std::numeric_limits< size_t >::max() / sizeof(Circle))
  {
    throw std::length_error("Too many circles");
  }

  const Request request{RequestType::start, id, threads, tries, circles.size()};

  if (!writeAll(process.command_fd, &request, sizeof(request)))
  {
    process.alive = false;
    throw std::runtime_error("Cannot send calculation request");
  }

  if (!writeAll(process.command_fd, circles.data(), circles.size() * sizeof(Circle)))
  {
    process.alive = false;
    throw std::runtime_error("Cannot send circles");
  }

  CalcResponse response{};

  if (!readAll(process.response_fd, &response, sizeof(response)) || response.id != id)
  {
    process.alive = false;
    throw std::runtime_error("Cannot receive calculation response");
  }

  return response;
}

malashenko::CalcResponse malashenko::queryCalc(Process& process, size_t id, bool wait)
{
  if (!process.alive)
  {
    throw std::runtime_error("Process is not alive");
  }

  const Request request{wait ? RequestType::wait : RequestType::status, id, 0, 0, 0};

  if (!writeAll(process.command_fd, &request, sizeof(request)))
  {
    process.alive = false;
    throw std::runtime_error("Cannot send status request");
  }

  CalcResponse response{};

  if (!readAll(process.response_fd, &response, sizeof(response)) || response.id != id)
  {
    process.alive = false;
    throw std::runtime_error("Cannot receive status");
  }

  return response;
}

void malashenko::closeProcess(Process& process)
{
  process.alive = false;

  if (process.command_fd >= 0)
  {
    close(process.command_fd);
    process.command_fd = -1;
  }

  if (process.response_fd >= 0)
  {
    close(process.response_fd);
    process.response_fd = -1;
  }

  if (process.pid > 0)
  {
    while (waitpid(process.pid, nullptr, 0) == -1)
    {
      if (errno != EINTR)
      {
        break;
      }
    }

    process.pid = -1;
  }
}
