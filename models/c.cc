#include "models/c.h"
using namespace std;

template<pANTLR3_BASE_TREE>
void partitionList(list<pANTLR3_BASE_TREE> src, list<pANTLR3_BASE_TREE> &yes, list<pANTLR3_BASE_TREE> &no, bool (*predicate)(pANTLR3_BASE_TREE) );

template<pANTLR3_BASE_TREE>
void takeWhile(list<pANTLR3_BASE_TREE>::iterator &it, list<pANTLR3_BASE_TREE>::iterator end, list<pANTLR3_BASE_TREE> &target, bool (*predicate)(pANTLR3_BASE_TREE));

Type::Type( ) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1), typedefName(NULL), retType(NULL), nFields(0), fields(NULL),
  isEnum(false)
{
}

// Just for convenience
Type::Type( list<pANTLR3_BASE_TREE>::iterator start, list<pANTLR3_BASE_TREE>::iterator end) :
  isStatic(false), isExtern(false), isTypedef(false), isAuto(false), isUnsigned(false),
  isFunction(false), isRegister(false), isConst(false), primType(-1), stars(0), array(0),
  params(NULL), nParams(-1), typedefName(NULL), retType(NULL), nFields(0), fields(NULL),
  isEnum(false)
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
      case STAR:
        stars++;
        break;
      case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID: case SHORT:
        primType = tokT;
        break;
      case UNSIGNED :  isUnsigned = true; break;
      case AUTO:       isAuto = true;     break;
      case TYPEDEF:    isTypedef = true;  break;
      case EXTERN:     isExtern = true;   break;
      case STATIC:     isStatic = true;   break;
      case CONST:      isConst  = true;   break;
      case VOLATILE:                      break;
      case INLINE:                        break;
      case ELLIPSIS:                      break;
      case DECL: 
        return;        // Type tokens are prior to declarators
      case IDENT:
        primType = TYPEDEF;
        typedefName = (char*)tok->getText(tok)->chars;
        break;
      case STRUCT:
        // If there is an IDENT inside then store the name, else anonymous
        // Build the field list by parsing the declarations
        isStruct = true;
        break;
      case ENUM :
        isEnum = true;
        break;
      default:
        if( tok->getText(tok) != NULL )
          printf("Type::parse unknown --> %s %d\n", (char*)tok->getText(tok)->chars, tokT);
        else
          printf("Type::parse unknown --> empty %d\n", tokT);
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
    case TYPEDEF :
      prefix.push_back(typedefName);    // An instance of a typedef'd name, not a typedef declaration
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

bool isNotDecl(pANTLR3_BASE_TREE tok)
{
  return tok->getType(tok) != DECL;
}

/* The subtree for a DECL can contain multiple declarations in a comma-separated
   list. The initial children specify the type, these will be cloned into every
   Decl produced. The results are appended to the list<Decl*> passed in.
*/
void Decl::parse(pANTLR3_BASE_TREE node, list<Decl*> &results)
{
  TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  //partitionList(children, typeToks, dtorToks, isTypeTok);
  partitionList(children, typeToks, dtorToks, isNotDecl);

// This is where the identifier/typename ambiguity would bite us, but the symbol name IDENTS are
// wrapped inside DECL nodes (one for each dtor in the declaration). 
  Type baseType(typeToks.begin(), typeToks.end());    // Each dtor can contain stars or prototypes.

  /*printf("typeToks: ");
  printTokList(typeToks);
  printf("dtorToks: ");
  printTokList(dtorToks);*/

  // If no valid typeSpecifier then expect an IDENT inside the type part.
  /*  Moved into Type::parse
      means that all IDENTS in the type block will be treated as typedef'd names
  if(baseType.primType==-1)
  {
    tmplForeach(list, pANTLR3_BASE_TREE, tok, typeToks)
      if( tok->getType(tok)==IDENT )
      {
        baseType.typedefName = (char*)tok->getText(tok)->chars;
        baseType.primType = TYPEDEF;
      }
    tmplEnd
    if( baseType.typedefName == NULL)
      printf("Invalid type specification - no primitive or typedef supplied\n");
  }*/

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
      //printf("About to do dtors:\n");
      //dumpTree(subTree,1);
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
        break;
      /* ********* Leave this out until we parse expressions


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
        break;*/
      case ASSIGN:
        return;      // Skip initialiser expressions
      // A parenthesised tail to a declarator
      case DECLPAR:
      {
        TokList parCnts = extractChildren(tok, 0, -1);
        // Check first (counts as a side-effect) to ease initialisation in array
        tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
          if( parTok->getType(parTok) != PARAM && parTok->getType(parTok) != ELLIPSIS)
            throw BrokenTree(tok,"Non-PARAM node inside a declTail parenthesized block");
        tmplEnd
        type.nParams = parCnts.size();
        type.params = new Type[ type.nParams ];
        type.paramNames = new char *[ type.nParams ];

        int i=0;
        tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
          if( parTok->getType(parTok) == ELLIPSIS )
            type.paramNames[i] = "...";
          else
            type.paramNames[i] =  parseParam(parTok, &type.params[i]);
          i++;
        tmplEnd
        type.isFunction = true;
        break;
      }
      default:
        dumpTree(subTree,0);
        throw BrokenTree(tok, "Unexpected child type in subtree:");
    }
  }
}

/* This is where we resolve typenames / symbol names. Until now both are flushed down from the
   grammar as IDENTs. Normally people expend a huge amount of effort on threading a symbol
   table throughout every rule in the grammar to make sure they can tell these two types of
   tokens apart (context-sensitive so cannot be handled with a vanilla CFG). Instead we put
   the complexity in this one place where we can count the IDENTs in a stream and use a simple
   state machine to do the separation.
*/
void findTypeTokens(TokList stream, TokList &typeToks, TokList &rest)
{
int idCount = 0;
bool prefix = true;
  tmplForeach(list, pANTLR3_BASE_TREE, tok, stream)
    if(prefix)
    {
      switch(tok->getType(tok))
      {
        case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID: case STRUCT:
        case SHORT: case ENUM:
          idCount = 1;    // Pretend we saw the ident
          break;
        case UNSIGNED: case AUTO:   case TYPEDEF: case EXTERN: case STATIC:
        case VOLATILE: case CONST:  
          break;        // Continue in prefix
        case IDENT:
          if( idCount++ > 0 )
            prefix = false;
          break;
        default:
          prefix = false;
          break;
      }
    }
    if(prefix)
      typeToks.push_back(tok);
    else
      rest.push_back(tok);
  tmplEnd
}

/* Parameters are either type signatures (prototypes), or types + names (prototypes and 
   definitions). We can represent the type+name combination as a Decl, and use NULL
   identifiers for the anonymous cases.
*/
char *parseParam(pANTLR3_BASE_TREE node, Type *target)
{
  TokList children = extractChildren(node,0,-1);
  //if( children.size() < 2 )
  //  throw BrokenTree(node, "Illegal parameter, too few children");

  // Process: typeWrapper typeQualifier?
  list<pANTLR3_BASE_TREE> typeToks, others;
  findTypeTokens(children, typeToks, others);
  //splitList<pANTLR3_BASE_TREE>(children, typeToks, others, isTypeTok);
  target->parse(typeToks.begin(), typeToks.end());

  // Process: STAR*
  TokList::iterator walk = others.begin();
  target->stars = 0;
  while(walk!=others.end())
  {
    ANTLR3_UINT32 nType = (*walk)->getType(*walk);
    if (nType==STAR)
      target->stars++;
    else if(nType==CONST)
      ;
    else 
      break;
    ++walk;
  }

  // Process: IDENT?
  if( walk!=others.end() )
  {
    pANTLR3_BASE_TREE idTok = *walk;
    if( idTok->getType(idTok) == FPTR )
    {
      // TODO: Merge this with the other FPTR processing block
      TokList fpChildren = extractChildren(idTok, 0, -1);
      if( fpChildren.size()<2 )
        throw BrokenTree(idTok, "FPTR without enough children");
      pANTLR3_BASE_TREE id2Tok = *(++fpChildren.begin());
      if( id2Tok->getType(id2Tok)!=IDENT )
        throw BrokenTree(idTok, "FPTR did not contain IDENT");
      target->isFunction = true;
      target->retType = new Type();
      target->retType->stars = target->stars;
      target->stars = 1;
      target->retType->primType = target->primType;
      target->primType = FPTR;
      return (char *)id2Tok->getText(id2Tok)->chars;
    }
    if( idTok->getType(idTok) != IDENT )
    {
      dumpTree(idTok,0);
      throw BrokenTree(node, "Non-IDENT following STARs in paramter");
    }
    return (char*)idTok->getText(idTok)->chars;
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
  /* Once upon a time these parameters were parsed as declarations, back when the world was young and
     the token stream was flat. But it because necessary to indicate dtor boundaries so that IDENTs
     could be resolved into symbol names and typedef'd names. Hence the slightly ugly construction here */
  tmplForeach( list, pANTLR3_BASE_TREE, p, params)
    Type dummy;
    char *name = parseParam(p, &dummy);
    Decl *d = new Decl(dummy);
    d->identifier = name;
    args.push_back(d);
  tmplEnd
}
