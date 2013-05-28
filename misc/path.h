#ifndef PATH_H
#define PATH_H

#include<list>
#include<string>
class Path
{
std::list<std::string> components;
  std::string filename, ext;
  int ups;
  bool relative;
public:
  Path(std::string const &);
  bool inside(const Path &) const;
  std::string repr() const;
};

#endif
