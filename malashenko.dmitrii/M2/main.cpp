#include <iostream>
#include <csignal>
#include <signal.h>
#include <unordered_map>
#include <limits>
#include "application.hpp"
#include "commands.hpp"
#include "string_buf.hpp"

int main()
{
  using namespace malashenko;

  using command_t = void (*)(std::istream&, std::ostream&, ApplicationState&);

  std::unordered_map< std::string, command_t > commands;

  commands["circle"] = doCircle;
  commands["set"] = doSet;
  commands["show"] = doShow;
  commands["showset"] = doShowset;
  commands["frame"] = doFrame;
  commands["frameset"] = doFrameset;
  commands["spawn"] = doSpawn;
  commands["area-on"] = doAreaOn;
  commands["status"] = doStatus;
  commands["wait"] = doWait;

  ApplicationState state;
  std::string line;

  while (std::getline(std::cin, line))
  {
    if (!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }

    if (line.empty())
    {
      continue;
    }

    const size_t pos = line.find_first_of(" \t");

    const std::string command = (pos == std::string::npos) ? line : line.substr(0, pos);

    const std::string arguments = (pos == std::string::npos) ? "" : line.substr(pos);

    try
    {
      StringBuf sbuf(arguments);
      std::istream iss(&sbuf);

      const command_t cmd = commands.at(command);
      cmd(iss, std::cout, state);

      iss >> std::ws;
      if (iss.peek() != std::char_traits< char >::eof())
      {
        throw std::invalid_argument("trailing arguments");
      }
    }
    catch (std::out_of_range& e)
    {
      std::cout << "<UNKNOWN COMMAND>" << '\n';
    }
    catch (std::invalid_argument& e)
    {
      std::cout << "<INVALID COMMAND>" << '\n';
    }
    catch (std::runtime_error& e)
    {
      std::cout << e.what() << '\n';
    }
  }

  return 0;
}

