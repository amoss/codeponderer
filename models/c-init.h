#include <exception>
#include "models/util.h"

/*
class Stmt
{
public:
  std::list<Stmt*> children;
  std::list<Expr*> expressions;
  enum { If, While, For, Return, Other } type;
  Stmt( pANTLR3_BASE_TREE node );
};*/


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
