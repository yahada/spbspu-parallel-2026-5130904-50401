#include "commands.hpp"
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

#include "application.hpp"
#include "process.hpp"

void malashenko::showCircle(const Circle& circle, std::ostream& out)
{
  out << circle.radius << " (" << circle.x << " " << circle.y << ")" << '\n';
}
void malashenko::showBox(const BoundingBox& box, std::ostream& out)
{
  out << "(" << box.min_x << " " << box.min_y << ")" << " (" << box.max_x << " " << box.max_y << ")" << '\n';
}

void malashenko::printCalculationStatus(const malashenko::Calculation& calculation, std::ostream& out)
{
  using namespace malashenko;

  if (calculation.status == CalcStatus::in_progress)
  {
    out << calculation.process_name << " <IN PROGRESS>\n";
    return;
  }

  if (calculation.status == CalcStatus::failed)
  {
    throw std::runtime_error("Calculation failed");
  }

  const std::ios::fmtflags flags = out.flags();
  const std::streamsize precision = out.precision();

  out << calculation.process_name << ' ' << std::fixed << std::setprecision(3) << calculation.milliseconds << ' '
      << calculation.area << '\n';

  out.flags(flags);
  out.precision(precision);
}

void malashenko::updateCalculation(ApplicationState& state, Calculation& calculation, bool should_wait)
{
  using namespace malashenko;

  auto process_it = state.processes.find(calculation.process_name);

  if (process_it == state.processes.end() || !process_it->second.alive)
  {
    throw std::runtime_error("Process is unavailable");
  }

  const CalcResponse response = queryCalc(process_it->second, calculation.id, should_wait);

  calculation.status = response.status;
  calculation.milliseconds = response.milliseconds;
  calculation.area = response.area;
}

void malashenko::doCircle(std::istream& in, std::ostream&, ApplicationState& state)
{
  std::string name;
  int radius = 0, x = 0, y = 0;

  if (!(in >> name >> radius >> x >> y))
  {
    throw std::invalid_argument("Invalid arguments");
  }

  malashenko::createCircle(state, name, radius, x, y);
}

void malashenko::doSet(std::istream& in, std::ostream&, ApplicationState& state)
{
  std::string name;
  long long size_input = 0;

  if (!(in >> name >> size_input) || size_input <= 0)
  {
    throw std::invalid_argument("Invalid arguments");
  }

  const size_t size = static_cast< size_t >(size_input);

  if (size == 0)
  {
    throw std::invalid_argument("Invalid arguments");
  }

  std::vector< std::string > circle_names;
  circle_names.reserve(size);

  for (size_t i = 0; i < size; ++i)
  {
    std::string circle_name;
    if (!(in >> circle_name))
    {
      throw std::invalid_argument("Invalid circle name");
    }

    circle_names.push_back(circle_name);
  }

  createSet(state, name, circle_names);
}

void malashenko::doShow(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string circle_name;

  if (!(in >> circle_name))
  {
    throw std::invalid_argument("Invalid circle name");
  }

  if (!state.circles.count(circle_name))
  {
    throw std::invalid_argument("Invalid circle name");
  }

  showCircle(getCircle(state, circle_name), out);
}

void malashenko::doShowset(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string set_name;

  if (!(in >> set_name))
  {
    throw std::invalid_argument("Invalid set name");
  }

  if (!state.sets.count(set_name))
  {
    throw std::invalid_argument("Unknown set name");
  }

  const std::vector< std::string >& set = getSet(state, set_name);
  for (size_t i = 0; i < set.size(); ++i)
  {
    showCircle(getCircle(state, set[i]), out);
  }
}

void malashenko::doFrame(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string circle_name;

  if (!(in >> circle_name))
  {
    throw std::invalid_argument("Invalid circle name");
  }

  if (!state.circles.count(circle_name))
  {
    throw std::invalid_argument("Invalid circle name");
  }

  showBox(getCircleFrame(state, circle_name), out);
}

void malashenko::doFrameset(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string set_name;

  if (!(in >> set_name))
  {
    throw std::invalid_argument("Invalid set name");
  }

  if (!state.sets.count(set_name))
  {
    throw std::invalid_argument("Unknown set name");
  }

  showBox(getSetFrame(state, set_name), out);
}

void malashenko::doSpawn(std::istream& in, std::ostream&, ApplicationState& state)
{
  std::string name;
  long long seed_input = 0;

  if (!(in >> name >> seed_input))
  {
    throw std::invalid_argument("Invalid arguments");
  }

  if (seed_input < 0)
  {
    throw std::invalid_argument("Invalid process");
  }

  const size_t seed = static_cast< size_t >(seed_input);

  if (state.processes.count(name) != 0)
  {
    throw std::invalid_argument("Invalid process");
  }

  std::vector< Process > existing_processes;
  existing_processes.reserve(state.processes.size());

  for (const auto& entry : state.processes)
  {
    existing_processes.push_back(entry.second);
  }

  Process process = spawnProcess(seed, existing_processes);

  try
  {
    state.processes.emplace(name, process);
  }
  catch (...)
  {
    closeProcess(process);
    throw;
  }
}

void malashenko::doAreaOn(std::istream& in, std::ostream&, ApplicationState& state)
{
  std::string process_name;
  std::string calc_name;
  std::string set_name;

  long long threads_input = 0;
  long long tries_input = 0;

  if (!(in >> process_name >> calc_name >> set_name >> threads_input >> tries_input))
  {
    throw std::invalid_argument("Invalid arguments");
  }

  if (threads_input <= 0 || tries_input <= 0)
  {
    throw std::invalid_argument("Invalid calculation");
  }

  const size_t threads = static_cast< size_t >(threads_input);
  const size_t tries = static_cast< size_t >(tries_input);

  if (state.calculations.count(calc_name) != 0)
  {
    throw std::invalid_argument("Invalid calculation");
  }

  auto process_it = state.processes.find(process_name);

  if (process_it == state.processes.end() || !process_it->second.alive)
  {
    throw std::invalid_argument("Process does not exist");
  }

  const std::vector< Circle > circles = collectSetCircles(state, set_name);

  const size_t id = state.next_calculation_id++;

  Calculation calculation{process_name, id, CalcStatus::in_progress, 0.0, 0.0};

  auto calculation_it = state.calculations.emplace(calc_name, calculation);

  try
  {
    const CalcResponse response = startCalc(process_it->second, id, circles, threads, tries);

    if (response.status != CalcStatus::in_progress)
    {
      throw std::runtime_error("Calculation was not started");
    }
  }
  catch (...)
  {
    state.calculations.erase(calculation_it.first);
    throw;
  }
}

void malashenko::doStatus(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string calc_name;

  if (!(in >> calc_name))
  {
    throw std::invalid_argument("Invalid calculation name");
  }

  auto calculation_it = state.calculations.find(calc_name);

  if (calculation_it == state.calculations.end())
  {
    throw std::invalid_argument("Calculation does not exist");
  }

  updateCalculation(state, calculation_it->second, false);

  printCalculationStatus(calculation_it->second, out);
}

void malashenko::doWait(std::istream& in, std::ostream& out, ApplicationState& state)
{
  std::string calc_name;

  if (!(in >> calc_name))
  {
    throw std::invalid_argument("Invalid calculation name");
  }

  auto calculation_it = state.calculations.find(calc_name);

  if (calculation_it == state.calculations.end())
  {
    throw std::invalid_argument("Calculation does not exist");
  }

  updateCalculation(state, calculation_it->second, true);

  printCalculationStatus(calculation_it->second, out);
}
