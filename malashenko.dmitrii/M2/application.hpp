#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <map>
#include <string>
#include <vector>
#include "geometry.hpp"
#include "process.hpp"

namespace malashenko
{
  struct Calculation
  {
    std::string process_name;
    std::size_t id;

    CalcStatus status;

    double milliseconds;
    double area;
  };

  struct ApplicationState
  {
    std::map< std::string, Circle > circles;

    std::map< std::string, std::vector< std::string > > sets;

    std::map< std::string, Process > processes;

    std::map< std::string, Calculation > calculations;

    size_t next_calculation_id = 1;
  };

  void createCircle(ApplicationState& state, const std::string& name, int radius, int x, int y);

  void createSet(ApplicationState& state, const std::string& name, const std::vector< std::string >& circle_names);

  const Circle& getCircle(const ApplicationState& state, const std::string& name);

  const std::vector< std::string >& getSet(const ApplicationState& state, const std::string& name);

  std::vector< Circle > collectSetCircles(const ApplicationState& state, const std::string& name);

  BoundingBox getCircleFrame(const ApplicationState& state, const std::string& name);

  BoundingBox getSetFrame(const ApplicationState& state, const std::string& name);
}

#endif
