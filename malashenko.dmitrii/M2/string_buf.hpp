#ifndef STRING_BUF_HPP
#define STRING_BUF_HPP

#include <streambuf>
#include <string>

namespace malashenko
{
  class StringBuf: public std::streambuf
  {
  public:
    explicit StringBuf(const std::string& str)
    {
      char* const begin = const_cast< char* >(str.data());
      setg(begin, begin, begin + str.size());
    }
  };
}

#endif
