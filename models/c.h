#include <exception>
#include "models/util.h"

class Type
{
public:
  bool isStatic, isExtern, isTypedef, isAuto, isRegister, isUnsigned, isFunction, isConst;
  int  primType; 
  int stars;
  int array;
  int nParams;
  char *typedefName;
  Type *params;
  char **paramNames;
  Type *retType;
  Type();
  Type( TokList::iterator start, TokList::iterator end);
  void parse(TokList::iterator start, TokList::iterator end);
  std::string str();
};

class Decl
{
public:
  Type type;
  char *identifier;

  Decl(Type &t);
  static void parse(pANTLR3_BASE_TREE node, std::list<Decl*> &results);
  void parseInitDtor(pANTLR3_BASE_TREE subTree);
};

char *parseParam(pANTLR3_BASE_TREE node, Type *target);

class FuncDef
{
public:
  Type retType;
  std::list<Decl*> args;
  char *identifier;
  FuncDef();
  void parse(pANTLR3_BASE_TREE node);
};

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
