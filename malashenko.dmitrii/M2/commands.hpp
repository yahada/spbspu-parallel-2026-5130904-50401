#ifndef COMMANDS_HPP
#define COMMANDS_HPP
#include "application.hpp"

namespace malashenko
{

  void doCircle(std::istream& in, std::ostream&, ApplicationState& state);
  void doSet(std::istream& in, std::ostream&, ApplicationState& state);
  void doShow(std::istream& in, std::ostream&, ApplicationState& state);
  void doShowset(std::istream& in, std::ostream&, ApplicationState& state);
  void doFrame(std::istream& in, std::ostream&, ApplicationState& state);
  void doFrameset(std::istream& in, std::ostream&, ApplicationState& state);
  void doSpawn(std::istream& in, std::ostream&, ApplicationState& state);
  void doAreaOn(std::istream& in, std::ostream&, ApplicationState& state);
  void doStatus(std::istream& in, std::ostream&, ApplicationState& state);
  void doWait(std::istream& in, std::ostream&, ApplicationState& state);

  void showCircle(const Circle& circle, std::ostream&);
  void showBox(const BoundingBox& box, std::ostream&);
  void updateCalculation(ApplicationState& state, Calculation& calculation, bool should_wait);
  void printCalculationStatus(const malashenko::Calculation& calculation, std::ostream& out);

}

#endif
