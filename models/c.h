#include "models/util.h"

class Type
{
public:
  bool isStatic, isExtern, isTypedef, isAuto, isRegister, isUnsigned, isFunction, isConst;
  int  primType;    // Return type for functions
  int stars;
  int array;
  int nParams;
  char *typedefName;
  Type *params;
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

class FuncDef
{
public:
  Type retType;
  std::list<Decl*> args;
  char *identifier;
  FuncDef();
  void parse(pANTLR3_BASE_TREE node);
};

