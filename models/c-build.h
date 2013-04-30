#include <list>
#include <exception>
#include "models/util.h"
#include "models/c-final.h"

TranslationU parseUnit(char *filename);

class BrokenTree : public std::exception
{
public:
  pANTLR3_BASE_TREE blame;
  const char *explain;
  BrokenTree(pANTLR3_BASE_TREE node, const char *text)
    :  blame(node), explain(text)
  {
  }
  const char *what() const throw () { return explain; }
};

