#include <exception>
#include "models/util.h"

class Type
{
public:
  bool isStatic, isExtern, isTypedef, isAuto, isRegister, isUnsigned, isFunction, isConst, 
       isStruct, isEnum;
  int  primType; 
  int stars;
  int array;
  int nParams;
  char *typedefName;
  Type *params;
  char **paramNames;
  int nFields;
  Type *fields;
  char **fieldNames;
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

/*class Expression
{
  Expression( pANTLR3_BASE_TREE node ) ;
};

class Stmt
{
public:
  std::list<Stmt*> children;
  std::list<Expr*> expressions;
  enum { If, While, For, Return, Other } type;
  Stmt( pANTLR3_BASE_TREE node );
};*/

class FuncDef
{
public:
  Type retType;
  std::list<Decl*> args;
  char *identifier;
  FuncDef();
  std::list<pANTLR3_BASE_TREE> stmtNodes;
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

class TranslationU
{
public:
  std::list<Decl*> globals;
  std::list<FuncDef*> functions;
  TranslationU(pANTLR3_BASE_TREE root);
  void processTopLevel(pANTLR3_BASE_TREE node);
  void dump();
};
