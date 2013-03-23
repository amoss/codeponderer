#include "models/util.h"

class Type
{
public:
  bool isStatic, isExtern, isTypedef, isAuto, isRegister, isUnsigned, isFunction, isConst;
  int  primType;    // Return type for functions
  int stars;
  int array;
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

/*  void parseParam(pANTLR3_BASE_TREE node)
  {
    int count = node->getChildCount(node);
    if( count<2 ) {
      printf("Illegal parameter in function definition\n");
      dumpTree(node,0);
      return;
    }
    list<pANTLR3_BASE_TREE> children = extractChildren(node,0,-1);
    //list<pANTLR3_BASE_TREE> types,names;
    //splitList<pANTLR3_BASE_TREE>(children,types,names,isType);
    parseSpecifiers(children.begin(), children.end());  // children after specifiers
    for(int i=0; i<count; i++)
      if(getChildType(node,i)==DECL)
      {
        parseInits((pANTLR3_BASE_TREE)node->getChild(node,i));
        break;
      }
    //pANTLR3_BASE_TREE idTok = *(names.begin());
    //identifier = (char*)idTok->getText(idTok)->chars;

  }
*/

class FuncDef
{
};

