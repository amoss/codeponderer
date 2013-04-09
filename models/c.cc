#include "models/c.h"
using namespace std;

template<pANTLR3_BASE_TREE>
void partitionList(list<pANTLR3_BASE_TREE> src, list<pANTLR3_BASE_TREE> &yes, list<pANTLR3_BASE_TREE> &no, bool (*predicate)(pANTLR3_BASE_TREE) );

template<pANTLR3_BASE_TREE>
void takeWhile(list<pANTLR3_BASE_TREE>::iterator &it, list<pANTLR3_BASE_TREE>::iterator end, list<pANTLR3_BASE_TREE> &target, bool (*predicate)(pANTLR3_BASE_TREE));

Type::Type( ) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1), typedefName(NULL), retType(NULL)
{
}

// Just for convenience
Type::Type( list<pANTLR3_BASE_TREE>::iterator start, list<pANTLR3_BASE_TREE>::iterator end) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1), typedefName(NULL), retType(NULL)
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
    case FPTR :
      prefix.push_back("<fp>");
      break;
    case -1:
      prefix.push_back("NONE");
      break;
  }
  stringstream res;
  res << joinStrings(prefix,' ');
  if(isFunction)
  {
    if(retType!=NULL)
      res << retType->str();
    res << "(";
    list<string> paramTs;
    for(int i=0; i<nParams; i++)
      paramTs.push_back( params[i].str() );
    res << joinStrings(paramTs,',');
    res << ")";
  }
  else 
    for(int i=0; i<stars; i++)
      res << '*';
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

  // If no valid typeSpecifier then consume one IDENT as a typedef
  if(baseType.primType==-1)
  {
    pANTLR3_BASE_TREE custom = *(dtorToks.begin());
    if( custom->getType(custom) != IDENT )
      printf("Invalid type specification - no primitive or typedef supplied\n");
    else
    {
      baseType.typedefName = (char*)custom->getText(custom)->chars;
      dtorToks.pop_front();
    }
  }

  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    Decl *d = new Decl(baseType);
    d->parseInitDtor(dtor);
    results.push_back(d);
  tmplEnd
}

void Decl::parseInitDtor(pANTLR3_BASE_TREE subTree)
{
  TokList ptrQualToks, dtorToks, children = extractChildren(subTree,0,-1);

  // Separate the (STAR typeQualifier?)* prefix from the declarator
  partitionList(children, ptrQualToks, dtorToks, isPtrQual);

  // Check the declName first as function-ptr processing is a major difference from
  // other declarations.
  if( dtorToks.size()==0 ) 
    throw BrokenTree(subTree,"Malformed declaration - pointers but no id");

  pANTLR3_BASE_TREE idTok = *(dtorToks.begin());
  switch(idTok->getType(idTok))
  {
    // Process: ^(FPTR STAR IDENT declPar?)
    case FPTR:
    {
      TokList fpChildren = extractChildren(idTok, 0, -1);
      if( fpChildren.size()<2 )
        throw BrokenTree(idTok, "FPTR without enough children");
      pANTLR3_BASE_TREE id2Tok = *(++fpChildren.begin());
      if( id2Tok->getType(id2Tok)!=IDENT )
        throw BrokenTree(idTok, "FPTR did not contain IDENT");
      identifier = (char *)id2Tok->getText(id2Tok)->chars;
      type.stars = 1;
      type.isFunction = true;
      type.retType = new Type();
      type.retType->stars = 0;
      type.retType->primType = type.primType;
      type.primType = FPTR;
      tmplForeach(list,pANTLR3_BASE_TREE,ptr,ptrQualToks)
        if( ptr->getType(ptr)==STAR )
          type.retType->stars++;
      tmplEnd
      printf("About to do dtors:\n");
      dumpTree(subTree,1);
      break;
    }

    // Process: IDENT 
    case IDENT:
      identifier = (char*)idTok->getText(idTok)->chars;
      type.stars = 0;
      tmplForeach(list,pANTLR3_BASE_TREE,ptr,ptrQualToks)
        if( ptr->getType(ptr)==STAR )
          type.stars++;
      tmplEnd
      break;

    default :
      throw BrokenTree(subTree,"Malformed init-dtor - missing declName");
  }




  for(TokList::iterator tokIt = ++dtorToks.begin(); tokIt!=dtorToks.end(); ++tokIt)
  {
    pANTLR3_BASE_TREE tok = *tokIt;
    switch(tok->getType(tok))
    {
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
      // A parenthesised tail to a declarator
      case DECLPAR:
      {
        TokList parCnts = extractChildren(tok, 0, -1);
        // Check first (counts as a side-effect) to ease initialisation in array
        tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
          if( parTok->getType(parTok) != PARAM )
            throw BrokenTree(tok,"Non-PARAM node inside a declTail parenthesized block");
        tmplEnd
        type.nParams = parCnts.size();
        type.params = new Type[ type.nParams ];
        type.paramNames = new char *[ type.nParams ];

        int i=0;
        tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
          type.paramNames[i] =  parseParam(parTok, &type.params[i]);
          i++;
          //printf("PARAM inside declpar:\n");
          //dumpTree(decl,1);
        tmplEnd
        type.isFunction = true;
        break;
      }
      default:
        throw BrokenTree(tok, "Unexpected child type in subtree:");
    }
  }
}

/* Parameters are either type signatures (prototypes), or types + names (prototypes and 
   definitions). We can represent the type+name combination as a Decl, and use NULL
   identifiers for the anonymous cases.
*/
char *parseParam(pANTLR3_BASE_TREE node, Type *target)
{
  list<pANTLR3_BASE_TREE> children = extractChildren(node,0,-1);
  //if( children.size() < 2 )
  //  throw BrokenTree(node, "Illegal parameter, too few children");

  // Process: typeWrapper typeQualifier?
  list<pANTLR3_BASE_TREE> typeToks, others;
  splitList<pANTLR3_BASE_TREE>(children, typeToks, others, isTypeTok);
  target->parse(typeToks.begin(), typeToks.end());

  // Process: STAR*
  TokList::iterator walk = others.begin();
  target->stars = 0;
  while(walk!=others.end() && (*walk)->getType(*walk)==STAR) {
    target->stars++;
    ++walk;
  }

  // Process: IDENT?
  if( walk!=others.end() )
  {
    pANTLR3_BASE_TREE idTok = *walk;
    if( idTok->getType(idTok) != IDENT )
      throw BrokenTree(node, "Non-IDENT following STARs in paramter");
    return (char*)idTok->getText(idTok);
  }
  return NULL;
}


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
