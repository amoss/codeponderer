#include "models/c.h"
using namespace std;

template<pANTLR3_BASE_TREE>
void partitionList(std::list<pANTLR3_BASE_TREE> src, std::list<pANTLR3_BASE_TREE> &yes, std::list<pANTLR3_BASE_TREE> &no, bool (*predicate)(pANTLR3_BASE_TREE) );

Type::Type( ) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), primType(-1), stars(0), array(0)
{
}

// Just for convenience
Type::Type( list<pANTLR3_BASE_TREE>::iterator start, list<pANTLR3_BASE_TREE>::iterator end) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), primType(-1), stars(0), array(0)
{
  parse(start,end);
}

void Type::parse(TokList::iterator start, TokList::iterator end)
{
  while(start!=end)
  {
    pANTLR3_BASE_TREE tok = *start;
    int tokT = tok->getType(tok);
    switch(tokT)
    {
      case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID:
        primType = tokT;
        break;
      case UNSIGNED :  isUnsigned = true; break;
      case AUTO:       isAuto = true;     break;
      case TYPEDEF:    isTypedef = true;  break;
      case EXTERN:     isExtern = true;   break;
      case STATIC:     isStatic = true;   break;
      case DECL: 
        return;        // Type tokens are prior to declarators
      case CONST: 
        break;         // Ignore for now
      default:
        if( tok->getText(tok) != NULL )
          printf("decl-->%s %d\n", (char*)tok->getText(tok)->chars, tokT);
        else
          printf("decl-->empty %d\n", tokT);
        break;
    }
    ++start;
  }
}

string Type::str()
{
  list<string> prefix;
  if(isStatic)
    prefix.push_back("static");
  if(isExtern)
    prefix.push_back("extern");
  if(isTypedef)
    prefix.push_back("typedef");
  if(isAuto)
    prefix.push_back("auto");
  if(isRegister)
    prefix.push_back("register");
  if(isUnsigned)
    prefix.push_back("unsigned");
  switch(primType)
  {
    case CHAR:
      prefix.push_back("char");
      break;
    case FLOAT:
      prefix.push_back("float");
      break;
    case INT:
      prefix.push_back("int");
      break;
    case LONG:
      prefix.push_back("long");
      break;
  }
  stringstream res;
  res << joinStrings(prefix,' ');
  for(int i=0; i<stars; i++)
    res << '*';
  if(isFunction)
    res << "()";
  if(array>0)
    res << '[' << array << ']';
  return res.str();
}



Decl::Decl(Type &t) :
  type(t), identifier(NULL)
{
}

bool isTypeTok(pANTLR3_BASE_TREE tok)
{
  switch(tok->getType(tok)) 
  {
      case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID:
      case UNSIGNED: case AUTO:   case TYPEDEF: case EXTERN: case STATIC:
        return true;
      default:
        return false;
  }
}

bool isPtrQual(pANTLR3_BASE_TREE tok)
{
  switch(tok->getType(tok)) 
  {
    case CONST: case VOLATILE:  case STAR:
      return true;
    default:
      return false;
  }
}

/* The subtree for a DECL can contain multiple declarations in a comma-separated
   list. The initial children specify the type, these will be cloned into every
   Decl produced. The results are appended to the list<Decl*> passed in.
*/
void Decl::parse(pANTLR3_BASE_TREE node, list<Decl*> &results)
{
  TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  partitionList(children, typeToks, dtorToks, isTypeTok);

  Type baseType(typeToks.begin(), typeToks.end());    // Each dtor can contain stars or prototypes.

  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    Decl *d = new Decl(baseType);
    d->parseInitDtor(dtor);
    results.push_back(d);
  tmplEnd
}

void Decl::parseInitDtor(pANTLR3_BASE_TREE subTree)
{
  TokList ptrQualToks, dtorToks, children = extractChildren(subTree,0,-1);
  partitionList(children, ptrQualToks, dtorToks, isPtrQual);
  
  type.stars = 0;
  tmplForeach(list,pANTLR3_BASE_TREE,ptr,ptrQualToks)
    if( ptr->getType(ptr)==STAR )
      type.stars++;
  tmplEnd

  if( dtorToks.size()==0 ) {
    printf("Malformed declaration - pointers but no id\n");
    dumpTree(subTree,0);
    return;
  }

  pANTLR3_BASE_TREE idTok = *(dtorToks.begin());
  if( idTok->getType(idTok) != IDENT ) {
    printf("Malformed declaration - missing IDENT\n");
    dumpTree(subTree,0);
    return;
  }

  tmplForeach(list, pANTLR3_BASE_TREE, tok, ptrQualToks)
    switch(tok->getType(tok))
    {
      case IDENT:
        identifier = (char*)tok->getText(tok)->chars;
        break;
      case OPENSQ:
        if(distance(tokIt,ptrQualToks.end()) < 2)
          printf("ERROR: truncated array expression\n");
        else
        {
          ++tokIt;
          tok = *tokIt;
          if(tok->getType(tok)!=NUM)
            printf("ERROR: array bound is unevaluated\n");
          else
            type.array = atoi((char*)tok->getText(tok)->chars);
          // Skip NUM CLOSESQ
          if(++tokIt==ptrQualToks.end())
            printf("ERROR: truncated array expression\n");
        }
        break;
      case EQUALS:
        return;      // Skip initialiser expressions
      case OPENPAR:   // Prototype
        // upto CLOSEPAR
        //      list of DECLs
        //      do each on
        type.isFunction = true;
        return;
      default:
        printf("Unexpected child %d in subtree:\n", tok->getType(tok));
        dumpTree(subTree,0);
        return;
    }
  tmplEnd
}

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
