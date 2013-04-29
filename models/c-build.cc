#include "models/c-build.h"

using namespace std;

static void convertDECL(SymbolTable *st, pANTLR3_BASE_TREE node);
static string convertPARAM(SymbolTable *st, pANTLR3_BASE_TREE node, DataType *target);

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

// A declaration becomes ^(DECL declSpec+ initDtors) where each initDtor has a DECL root.
bool isNotDecl(pANTLR3_BASE_TREE tok)
{
  return tok->getType(tok) != DECL;
}

int countTokTypes(TokList toks, int tokType)
{
int result=0;
  tmplForeach(list,pANTLR3_BASE_TREE,t,toks)
    if( t->getType(t)==tokType )
      result++;
  tmplEnd
  return result;
}

pANTLR3_BASE_TREE findTok(TokList toks, int tokType)
{
  tmplForeach(list, pANTLR3_BASE_TREE, t, toks)
    if( t->getType(t)==tokType )
      return t;
  tmplEnd
  return NULL;
}


// Called from processing a  ^(DECL declSpec initDecl+)
//    declSpec contains storageClass, typeQualifier, typeSpecifier, IDENTs, INLINE keywords.
//    TokList was the tokens upto the first initDecl (DECL).
DataType convertDeclSpec(SymbolTable *st, TokList::iterator start, TokList::iterator end)
{
DataType result;
  while(start!=end)
  {
    pANTLR3_BASE_TREE tok = *start;
    int tokT = tok->getType(tok);
    switch(tokT)
    {
      case STAR:
        result.stars++;
        break;

      case DOUBLE:   result.primitive = DataType::Double; break;
      case FLOAT:    result.primitive = DataType::Float;  break;
      case CHAR:     result.primitive = DataType::Char;   break;
      case SHORT:    result.primitive = DataType::Short;  break;
      case LONG:     result.primitive = DataType::Long;   break;
      case INT:      result.primitive = DataType::Int;    break;
      case VOID:     result.primitive = DataType::Void;   break;

      case UNSIGNED: result.isUnsigned = true;            break;
      case AUTO:                                          break;
      case EXTERN:                                        break;
      case STATIC:                                        break;
      case CONST:                                         break;
      case VOLATILE:                                      break;
      case INLINE:                                        break;

      case TYPEDEF:                       break;    // TU???
      case ELLIPSIS:                      break;

      case IDENT:
        {
          map<string,const DataType*>::iterator it = st->typedefs.find((char*)tok->getText(tok)->chars);
          if(it==st->typedefs.end())
            throw BrokenTree(tok, "Unknown type");
          else
            return *it->second;
        }
      case STRUCT:
        printf("Type::parse(STRUCT)\n");
        dumpTree(tok,1);
        // If there is an IDENT inside then store the name, else anonymous
        // Build the field list by parsing the declarations
        ///isStruct = true;
        break;
      case ENUM :
        ///isEnum = true;
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
  return result;
}


// The structure of a record definition is:
//    ^((STRUCT|UNION) IDENT? DECL*)
// TODO: Not storing field names or order...
DataType convertRecord(TranslationU const &where, pANTLR3_BASE_TREE node)
{
DataType res;
  if( node->getType(node)==STRUCT )
    res.primitive = DataType::Struct;
  else 
    res.primitive = DataType::Union;

TokList cs = extractChildren(node,0,-1);
SymbolTable dummy;
  tmplForeach(list, pANTLR3_BASE_TREE, f, cs)
    if( f->getType(f) == DECL )
    {
      printf("field\n");
      convertDECL(&dummy,node);
    }
  tmplEnd
  res.nFields = dummy.symbols.size();
  res.fields  = new const DataType*[res.nFields];
  int idx = 0;
  // TODO: Fields will be in the wrong order iterating out of a map
  map<string,const DataType*>::iterator it = dummy.symbols.begin();
  for(; it!=dummy.symbols.end(); ++it)
    res.fields[idx++] = it->second;     // Cannonical for wrong table?!?
  return res;
}

/* Takes a declPar that is either a tail to a function-pointer or a function
   declaration. Shallow-copy is valid on FuncType objects as they do not own
   any of the DataType objects pointed to - they are the canonical instances
   from a SymbolTable. */
static FuncType convertParams(SymbolTable *st, pANTLR3_BASE_TREE tok)
{
FuncType result;
  // Count the PARAM children and check there are no non-PARAM children so that the
  // number is valid as an array allocation.
  TokList parCnts = extractChildren(tok, 0, -1);
  tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
    if( parTok->getType(parTok) != PARAM && parTok->getType(parTok) != ELLIPSIS)
      throw BrokenTree(tok,"Non-PARAM node inside a declTail parenthesized block");
  tmplEnd

  // Allocate storage
  result.nParams = parCnts.size();
  result.params  = new const DataType*[ result.nParams ];
  result.paramNames = new string[ result.nParams ];

  // Initialise parameters
  int i=0;
  tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
    if( parTok->getType(parTok) == ELLIPSIS )
    {
      result.paramNames[i] = "...";
      DataType t;
      t.primitive = DataType::Ellipsis;
      result.params[i] = st->getCanon(t); 
    }
    else
    {
      DataType temp;
      result.paramNames[i] = convertPARAM(st, parTok, &temp);
      result.params[i] = st->getCanon(temp);
    }
    i++;
  tmplEnd
  return result;
}

/* On calling the result is already initialised to a copy of the base-type from the 
   declaration. */
string convertInitDtor(SymbolTable *st, DataType &result, pANTLR3_BASE_TREE subTree)
{
char *identifier;
FuncType f;
TokList ptrQualToks, dtorToks, children = extractChildren(subTree,0,-1);

  // Separate the (STAR typeQualifier?)* prefix from the declarator
  partitionList(children, ptrQualToks, dtorToks, isPtrQual);
int numDeclPars = countTokTypes(dtorToks, DECLPAR);
  if(numDeclPars>1)
    throw BrokenTree(subTree,"Too many declPars - only one is valid");
  if(numDeclPars==1)
    f = convertParams( st, findTok(dtorToks, DECLPAR) );

  // Check the declName first as function-ptr processing is a major difference from
  // other declarations.
  if( dtorToks.size()==0 ) 
    throw BrokenTree(subTree,"Malformed declaration - pointers but no id");

  pANTLR3_BASE_TREE idTok = *(dtorToks.begin());
  switch(idTok->getType(idTok))
  {
    // Process: ^(FPTR STAR IDENT declPar?)
    // Build FuncType and link in to result
    case FPTR:
    {
      TokList fpChildren = extractChildren(idTok, 0, -1);
      if( fpChildren.size()<2 )
        throw BrokenTree(idTok, "FPTR without enough children");
      pANTLR3_BASE_TREE id2Tok = *(++fpChildren.begin());
      if( id2Tok->getType(id2Tok)!=IDENT )
        throw BrokenTree(idTok, "FPTR did not contain IDENT");
      identifier = (char *)id2Tok->getText(id2Tok)->chars;

      // There must be exactly one declPar
      if(numDeclPars==0)
        throw BrokenTree(subTree,"Function pointer with no parameters");

      DataType copy = *st->getCanon(result); 
      copy.stars += countTokTypes(ptrQualToks,STAR);
      f.retType = st->getCanon(result);
      result.fptr = st->getCanon(f);
      result.stars = 1;
      result.primitive = DataType::Function;
      break;
    }

    // Process: IDENT 
    case IDENT:
      identifier = (char*)idTok->getText(idTok)->chars;
      if(numDeclPars==1)
      {
        //DataType copy = *result.fptr->retType;
        result.stars += countTokTypes(ptrQualToks,STAR);
        f.retType = st->getCanon(result);
        result.primitive = DataType::Function;  // star
        result.fptr = st->getCanon(f);
        result.stars = 0;
      }
      else
      {
        result.stars += countTokTypes(ptrQualToks,STAR);
      }
      break;

    default :
      throw BrokenTree(subTree,"Malformed init-dtor - missing declName");
  }
  return identifier;
}

/* We do not get bitten by the identifier/typename ambiguity at this point because the
   IDENTS for symbol names are inside the DECL sub-tree for an initDtor while typenames
   are not. 
   Because a DECL can produce several declarations this function writes the results into
   the supplied SymbolTable, rather than producing a return value.
*/
static void convertDECL(SymbolTable *st, pANTLR3_BASE_TREE node)
{
  TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  partitionList(children, typeToks, dtorToks, isNotDecl);

  // Typedef IDENTs are converted during the convertDeclSpec call.
  DataType base = convertDeclSpec(st, typeToks.begin(), typeToks.end()); 
  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    DataType decl = base;
    string name = convertInitDtor(st, decl, dtor);
    const DataType *c = st->getCanon(decl) ;
    st->symbols[name] = c;
  tmplEnd
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

static string convertPARAM(SymbolTable *st, pANTLR3_BASE_TREE node, DataType *target)
{
  TokList children = extractChildren(node,0,-1);

  // Process: typeWrapper typeQualifier?
  list<pANTLR3_BASE_TREE> typeToks, others;
  findTypeTokens(children, typeToks, others);

  DataType base = convertDeclSpec(st, typeToks.begin(), typeToks.end()); 
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
  if( walk!=others.end() )
  {
    pANTLR3_BASE_TREE idTok = *walk;
    if( idTok->getType(idTok) == FPTR )
    {
      FuncType f = convertParams( st, findTok(others,DECLPAR) );
      TokList fpChildren = extractChildren(idTok, 0, -1);
      if( fpChildren.size()<2 )
        throw BrokenTree(idTok, "FPTR without enough children");
      pANTLR3_BASE_TREE id2Tok = *(++fpChildren.begin());
      if( id2Tok->getType(id2Tok)!=IDENT )
        throw BrokenTree(idTok, "FPTR did not contain IDENT");
      f.retType = st->getCanon(*target);
      target->fptr = st->getCanon(f);
      target->stars = 1;
      target->primitive = DataType::Function;
      return (char *)id2Tok->getText(id2Tok)->chars;
    }
    if( idTok->getType(idTok) != IDENT )
      throw BrokenTree(node, "Non-IDENT following STARs in paramter");
    *target = base;
    return (char*)idTok->getText(idTok)->chars;
  }
  else
  {
    *target = base;     // Prototype with no name
    return "";
  }
}

/*
static void convertFUNC(TranslationU &where, pANTLR3_BASE_TREE node, list<FuncDef*> &functions )
{

  FuncDef *f = new FuncDef;
  f->parse(node);
  functions.push_back(f);

}
*/





static void processTopLevel(pANTLR3_BASE_TREE node, TranslationU &tu)
{
int type  = node->getType(node);
int count = node->getChildCount(node);
  switch(type)
  {
    case PREPRO: return;
    case SEMI:   return;
    case DECL:   convertDECL(tu.table, node);         break;
    // case FUNC:   convertFUNC(tu, node, funcs);  break; 
    case FUNC: break;    // REMOVED ALL FUNCTION PROCESSING DURING REWRITE
    // Pure definition, build the type in the tag namespace
    case UNION: 
    case STRUCT:
    {
      pANTLR3_BASE_TREE idTok = (pANTLR3_BASE_TREE)node->getChild(node,0);
      string tag = (char*)idTok->getText(idTok)->chars;
      //list<Decl*> ordered;
      TokList fields = extractChildren((pANTLR3_BASE_TREE)node->getChild(node,1),0,-1);
      printf("Pure def: %s\n",tag.c_str());
      printTokList(fields);
      dumpTree(node,0);
      DataType r = convertRecord(tu, node);
      printf("%s\n", r.str().c_str());

    }
      break;
    default:
      printf("Unknown Type %u Children %u ", type, count);
      dumpTree(node,1);
      break;
  }
}

extern pANTLR3_UINT8   cInCParserTokenNames[];
void dropError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames)
{
}

/*bool testConvertDecl(Decl *d, SymbolTable *target)
{
  if( d->type.primType != TYPEDEF )
    return true;
  map<string,DataType*>::iterator it = target->typedefs.find(d->type.typedefName);
  if( it==target->typedefs.end() )
    return false;
  return true;
}*/

void dumpTokenStream(pANTLR3_COMMON_TOKEN_STREAM tokens)
{
  pANTLR3_VECTOR vec = tokens->getTokens(tokens);
  printf("%u\n", vec->elementsSize);
  for(int i=0; i<vec->elementsSize; i++)
  {
    pANTLR3_COMMON_TOKEN t = (pANTLR3_COMMON_TOKEN) vec->get(vec,i);
    if(t!=NULL)
      printf("%s(%u) ", cInCParserTokenNames[t->getType(t)], t->index);
  }
  printf("\n");
}

TranslationU parseUnit(char *filename)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return firstPass;
cInCParser_declaration_return retVal2;

/* First parse: resolve outer declarations and functions down to statement level (with statement parsetree
                in cases of typename ambiguity. */
  ip        = antlr3AsciiFileStreamNew((pANTLR3_UINT8)filename);
  // Todo: error checking for IO errors !
  lex       = cInCLexerNew(ip);
  tokens    = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  parser    = cInCParserNew(tokens);
  firstPass = parser->translationUnit(parser);

/* Pass II: semantic analysis on the partial parsetree */
//list<FuncDef*> functions;    REMOVED ALL FUNCTION PROCESSING DURING REWRITE
TranslationU result;
  /* Translation units with a single top-level declaration do not have a NIL node as parent. It only
     exists when the AST is a forest to act as a virtual root. */
  try {
    if( firstPass.tree->getType(firstPass.tree) == 0)
    {
      for(int i=0; i<firstPass.tree->getChildCount(firstPass.tree); i++)
        processTopLevel((pANTLR3_BASE_TREE)firstPass.tree->getChild(firstPass.tree,i), result
                        );
    }
    else
      processTopLevel(firstPass.tree, result);
  }
  catch(BrokenTree bt) {
    printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
    dumpTree(bt.blame,1);
  }


/* Second pass: check function statements to see if any were declarations that are legal in the context
                of the outer symbol table + the function symbol table as it is built. */

/* REMOVED ALL FUNCTION PROCESSING DURING REWRITE 

  // Dynamic overloading to prevent display of errors during speculation
  parser->pParser->rec->displayRecognitionError = dropError;
  //dumpTokenStream(tokens);

  tmplForeach(list, FuncDef*, f, functions)
    // Add a new Function to the SymbolTable...

    tmplForeach(list, pANTLR3_BASE_TREE, s, f->stmtNodes)
      // First token within the statement (virtual tokens have no position in the 
      // input stream)
      pANTLR3_COMMON_TOKEN tok;
      if( s->getType(s) == STATEMENT)
      {
        pANTLR3_BASE_TREE node2 = (pANTLR3_BASE_TREE)s->getChild(s,0);
        tok = node2->getToken(node2);
      }
      else
        tok = s->getToken(s);
      tokens->p = tok->getTokenIndex(tok);
      retVal2 = parser->declaration(parser);
      if( retVal2.tree->getType(retVal2.tree) == DECL )
      {
        // True iff all IDENTs were valid typenames (or primitives)
        // TODO: will not handle local typedefs, but must ensure that the SymbolTable update is atomic
        list<Decl*> vars;
        try {
          Decl::parse(retVal2.tree, vars);
          bool failed = false;
          tmplForeach(list, Decl*, v, vars)
            if(!testConvertDecl(v, result.table))      // st should be const during this loop
              failed = true;
          tmplEnd
          if(!failed)
          {
            printf("Statement valid as DECL:\n");
            dumpTree(retVal2.tree,0);
          }
          else
          {
            printf("Statement rejected as DECL\n");
            dumpTree(s,0);
          }
        }
        catch(BrokenTree bt)
        {
          printf("Exception in DECL processing - must be statement\n");
          dumpTree(s,0);
        }
      }
      else 
      {
        printf("Non-generic statement - can't be DECL\n");
        // Must be a statement if they were not.
        dumpTree(s,0);
      }
    tmplEnd
  tmplEnd
  */
  return result;
}
