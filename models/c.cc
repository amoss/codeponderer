#include "models/c.h"
using namespace std;

template<pANTLR3_BASE_TREE>
void partitionList(list<pANTLR3_BASE_TREE> src, list<pANTLR3_BASE_TREE> &yes, list<pANTLR3_BASE_TREE> &no, bool (*predicate)(pANTLR3_BASE_TREE) );

template<pANTLR3_BASE_TREE>
void takeWhile(list<pANTLR3_BASE_TREE>::iterator &it, list<pANTLR3_BASE_TREE>::iterator end, list<pANTLR3_BASE_TREE> &target, bool (*predicate)(pANTLR3_BASE_TREE));

Type::Type( ) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1)
{
}

// Just for convenience
Type::Type( list<pANTLR3_BASE_TREE>::iterator start, list<pANTLR3_BASE_TREE>::iterator end) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1)
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
      case CONST:      isConst  = true;   break;
      case DECL: 
        return;        // Type tokens are prior to declarators
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
  if(isConst)
    prefix.push_back("const");
  switch(primType)
  {
    case CHAR:
      prefix.push_back("char");
      break;
    case DOUBLE:
      prefix.push_back("double");
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
  {
    res << "(";
    list<string> paramTs;
    for(int i=0; i<nParams; i++)
      paramTs.push_back( params[i].str() );
    res << joinStrings(paramTs,',');
    res << ")";
  }
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
      case VOLATILE: case CONST:    // Part of the basetype if before initdtors
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

bool isParam(pANTLR3_BASE_TREE tok)
{
  return tok->getType(tok) == PARAM;
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

/* This pattern of splitting on a predicate is common enough that I might wrap
   this up later.
  printf("parseInit:\n");
  dumpTree(subTree,1);
  printf("partitions into:\nPtrQuals > ");
  printTokList(ptrQualToks);
  printf("Dtors > ");
  printTokList(dtorToks);
   */
  

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

  tmplForeach(list, pANTLR3_BASE_TREE, tok, dtorToks)
    switch(tok->getType(tok))
    {
      case IDENT:
        identifier = (char*)tok->getText(tok)->chars;
        break;
      case OPENSQ:
        if(distance(tokIt,dtorToks.end()) < 2)
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
      {
        type.isFunction = true;
        TokList params;
        takeWhile( ++tokIt, dtorToks.end(), params, isParam);
        type.params = new Type[ params.size() ];
        type.nParams = params.size();
        TokList::iterator p = params.begin();
        for(int i=0; i<type.nParams; i++)
        {
          list<pANTLR3_BASE_TREE> pChildren = extractChildren(*p, 0, -1);
          type.params[i].parse(pChildren.begin(), pChildren.end());
        }
        break;
      }
      case CLOSEPAR:
        break;    // Skip
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

FuncDef::FuncDef() :
  identifier(NULL)
{
}

void FuncDef::parse(pANTLR3_BASE_TREE node)
{
pANTLR3_BASE_TREE idTok = (pANTLR3_BASE_TREE)node->getChild(node,0);
  identifier = (char*)idTok->getText(idTok)->chars;
pANTLR3_BASE_TREE stmts = (pANTLR3_BASE_TREE)node->getChild(node,1);
  // Skip compound statement for now
TokList params;
TokList rest = extractChildren(node,2,-1);
TokList::iterator child = rest.begin();
  takeWhile( child, rest.end(), params, isParam);
  retType.parse(child, rest.end());
  tmplForeach( list, pANTLR3_BASE_TREE, p, params)
    Decl::parse(p, args);
  tmplEnd
}
