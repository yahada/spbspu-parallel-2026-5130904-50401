
#include "application.hpp"

void malashenko::createCircle(ApplicationState& state, const std::string& name, int radius, int x, int y)
{
  if (state.circles.count(name) != 0)
  {
    throw std::invalid_argument("Circle with that name is already exist");
  }

  if (name.empty() || !std::isalpha(static_cast< unsigned char >(name[0])))
  {
    throw std::invalid_argument("Invalid name");
  }

  const Circle circle{radius, x, y};

  if (radius <= 0)
  {
    throw std::invalid_argument("Invalid radius");
  }

  state.circles.emplace(name, circle);
}

void malashenko::createSet(
    ApplicationState& state, const std::string& name, const std::vector< std::string >& circle_names)
{
  if (name.empty())
  {
    throw std::invalid_argument("Name can't be empty");
  }

  if (state.sets.count(name) != 0 || circle_names.empty())
  {
    throw std::invalid_argument("Invalid set");
  }

  for (const std::string& circle_name : circle_names)
  {
    if (state.circles.count(circle_name) == 0)
    {
      throw std::invalid_argument("Circle does not exist");
    }
  }

  state.sets.emplace(name, circle_names);
}

const malashenko::Circle& malashenko::getCircle(const ApplicationState& state, const std::string& name)
{
  const auto it = state.circles.find(name);

  if (it == state.circles.end())
  {
    throw std::invalid_argument("Circle does not exist");
  }

  return it->second;
}

const std::vector< std::string >& malashenko::getSet(const ApplicationState& state, const std::string& name)
{
  const auto it = state.sets.find(name);

  if (it == state.sets.end())
  {
    throw std::invalid_argument("Set does not exist");
  }

  return it->second;
}

std::vector< malashenko::Circle > malashenko::collectSetCircles(const ApplicationState& state, const std::string& name)
{
  const std::vector< std::string >& names = getSet(state, name);

  std::vector< Circle > circles;
  circles.reserve(names.size());

  for (const std::string& circle_name : names)
  {
    circles.push_back(getCircle(state, circle_name));
  }

  return circles;
}

malashenko::BoundingBox malashenko::getCircleFrame(const ApplicationState& state, const std::string& name)
{
  return getBoundingBox(getCircle(state, name));
}

malashenko::BoundingBox malashenko::getSetFrame(const ApplicationState& state, const std::string& name)
{
  return getBoundingBox(collectSetCircles(state, name));
}
