#include "models/c-build.h"

using namespace std;

static void convertDECL(pANTLR3_BASE_TREE node, SymbolTable *st);
static void convertPARAM(pANTLR3_BASE_TREE node, Decl *target, SymbolTable *st);
TypeAtom convertRecord(pANTLR3_BASE_TREE node, SymbolTable *st);

#include "models/graph.cc"

bool isTypeTok(pANTLR3_BASE_TREE tok)
{
  switch(tok->getType(tok)) 
  {
      case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID:
      case UNSIGNED: case SIGNED: case AUTO:   case TYPEDEF: case EXTERN: case STATIC:
      case VOLATILE: case CONST:    // Part of the basetype if before initdtors
      case VALIST:
        return true;
      default:
        return false;
  }
}

bool isPtrQual(pANTLR3_BASE_TREE tok)
{
  switch(tok->getType(tok)) 
  {
    case CONST: case UCONST: case URESTRICT: case VOLATILE:  case STAR:
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


/* When we encounter a declPar after an ident in an init-dtor it indicates that what
   has been processed as a type so far is actually the return-type to a function.
   This function takes a FuncType initialised to contain the parameters and embeds
   the type as the function return type. The function type is registered in the
   SymbolTable and the TypeAtom updated to link to it as either a prototype (stars=0)
   or a function-pointer (stars=1).
*/
void swapReturn(FuncType f, TypeAtom *t, SymbolTable *st, int stars)
{
  f.retType = *t;
  t->primitive = TypeAtom::Function;
  t->stars = stars;
  t->fidx = st->savePrototype(f);
}

// Called from processing a  ^(DECL declSpec initDecl+)
//    declSpec contains storageClass, typeQualifier, typeSpecifier, IDENTs, INLINE keywords.
//    TokList was the tokens upto the first initDecl (DECL).
//   TODO: (fwd)
//     Decouple the parsing functions from the SymbolTable entirely. Return a list of Decl
//     All the injection / canonisation code moves to finalise
TypeAtom convertDeclSpec(TokList::iterator start, TokList::iterator end, 
                                TypeAnnotation &ann, SymbolTable *st)
{
TypeAtom result;
  while(start!=end)
  {
    pANTLR3_BASE_TREE tok = *start;
    int tokT = tok->getType(tok);
    switch(tokT)
    {
      case STAR:
        result.stars++;
        break;

      case DOUBLE:   result.primitive = TypeAtom::Double; break;
      case FLOAT:    result.primitive = TypeAtom::Float;  break;
      case CHAR:     result.primitive = TypeAtom::Char;   break;
      case SHORT:    result.primitive = TypeAtom::Short;  break;
      case LONG:     result.primitive = TypeAtom::Long;   break;
      case INT:      result.primitive = TypeAtom::Int;    break;
      case VOID:     result.primitive = TypeAtom::Void;   break;

      case UNSIGNED: result.isUnsigned = true;            break;
      case SIGNED:   /* default value */                  break;
      case AUTO:     ann.isAuto = true;                   break;
      case EXTERN:   ann.isExtern = true;                 break;
      case STATIC:   ann.isStatic = true;                 break;
      case CONST:    result.isConst = true;               break;
      case UCONST:   result.isConst = true;               break;
      case URESTRICT:                                     break;
      case VOLATILE: ann.isVolatile = true;               break;
      case INLINE:   ann.isInline   = true;               break;

      case TYPEDEF:  ann.isTypedef = true;                break;  
      case VALIST:  /* Todo: new primitive needed? */      break;
      case ELLIPSIS:                                      break;

      case IDENT:
        if(!st->validTypedef((char*)tok->getText(tok)->chars))
          throw BrokenTree(tok, "Unknown type");
        return st->getTypedef((char*)tok->getText(tok)->chars);
      case UNION:
      case STRUCT:
      {
        result = convertRecord(tok, st);
        break;
      }
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
//   TODO: (fwd)
//     Decouple the parsing functions from the SymbolTable entirely. Return a list of Decl
//     All the injection / canonisation code moves to finalise
TypeAtom convertRecord(pANTLR3_BASE_TREE node, SymbolTable *st)
{
TypeAtom res;
  //res.node = node;
  if( node->getType(node)==STRUCT )
    res.primitive = TypeAtom::Struct;
  else 
    res.primitive = TypeAtom::Union;

  // If there is an IDENT? then process it separately from the DECL loop
TokList cs = extractChildren(node,0,-1);
pANTLR3_BASE_TREE first = *cs.begin();
  if(cs.size()>0 && first->getType(first) == IDENT)
  {
    cs.pop_front();
    res.tag = (char*)first->getText(first)->chars;
  }
  else
    res.tag = st->anonName();

// When we call convertDECL it needs a SymbolTable to record the basetype+initdecl for each
// declarator in the declaration. We can't use the real TU table as these declarations exist
// in the private namespace of the record. To build the DataType that represents the record
// we can drop the names but we need the declaration DataTypes in the correct order...
// Type are canonical in private namespace... 

SymbolTable *recTable = NULL;
  tmplForeach(list, pANTLR3_BASE_TREE, f, cs)
    if( f->getType(f) == DECL )
    {
      if(recTable==NULL) recTable = new SymbolTable(st);
      convertDECL(f, recTable);
    }
  tmplEnd
  // If there is no compound then it is not a definition so skip updating the tag
  if( recTable!=NULL )
    st->saveRecord(res.tag, recTable);    
  return res;
}

/* Takes a declPar that is either a tail to a function-pointer or a function
   declaration. Shallow-copy is valid on FuncType objects as they do not own
   any of the DataType objects pointed to - they are the canonical instances
   from a SymbolTable. */
static FuncType convertParams(pANTLR3_BASE_TREE tok, SymbolTable *st, 
                              TokList parCnts=TokList())
{
FuncType result;
  // Count the PARAM children and check there are no non-PARAM children so that the
  // number is valid as an array allocation.
  if(parCnts.size()==0 && tok!=NULL)
  {
    parCnts = extractChildren(tok, 0, -1);
    tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
      if( parTok->getType(parTok) != PARAM && parTok->getType(parTok) != ELLIPSIS)
        throw BrokenTree(tok,"Non-PARAM node inside a declTail parenthesized block");
    tmplEnd
  }

  // Allocate storage
  result.nParams = parCnts.size();
  result.params  = new Decl[ result.nParams ];

  // Initialise parameters
  int i=0;
  tmplForeach(list, pANTLR3_BASE_TREE, parTok, parCnts)
    // Note: there is an unused ELLIPSIS case inside convertDeclSpec that is called from
    //       convertPARAM. It could simplify this to move it there.
    if( parTok->getType(parTok) == ELLIPSIS )
    {
      result.params[i].name = "...";
      result.params[i].type.primitive = TypeAtom::Ellipsis;
    }
    else
    {
      convertPARAM(parTok, &result.params[i], st);
    }
    i++;
  tmplEnd
  return result;
}

/* Start with a copy of the base-type from the initial tokens, add anything in the
   init-dtor onto it.
*/
Decl convertInitDtor(TypeAtom const &base, pANTLR3_BASE_TREE subTree, SymbolTable *st)
{
Decl result = Decl("",base);
FuncType f;
TokList ptrQualToks, dtorToks, children = extractChildren(subTree,0,-1);

  // Separate the (STAR typeQualifier?)* prefix from the declarator
  partitionList(children, ptrQualToks, dtorToks, isPtrQual);
int numDeclPars = countTokTypes(dtorToks, DECLPAR);
  if(numDeclPars>1)
    throw BrokenTree(subTree,"Too many declPars - only one is valid");
  if(numDeclPars==1)
    f = convertParams( findTok(dtorToks, DECLPAR), st );

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
      result.name = (char *)id2Tok->getText(id2Tok)->chars;

      // There must be exactly one declPar
      if(numDeclPars==0)
        throw BrokenTree(subTree,"Function pointer with no parameters");

      result.type.stars += countTokTypes(ptrQualToks,STAR);
      swapReturn(f, &result.type, st, 1);
      break;
    }

    // Process: IDENT 
    case IDENT:
      result.name = (char*)idTok->getText(idTok)->chars;
      // Prototype declaration
      if(numDeclPars==1)
      {
        result.type.stars += countTokTypes(ptrQualToks,STAR);
        swapReturn(f, &result.type, st, 0);
      }
      else
      {
        result.type.stars += countTokTypes(ptrQualToks,STAR);
      }
      break;

    default :
      printf("idTok\n");
      dumpTree(idTok,1);
      throw BrokenTree(subTree,"Malformed init-dtor - missing declName");
  }
  TokList::iterator it = dtorToks.begin();
  while(++it != dtorToks.end())
  {
    pANTLR3_BASE_TREE t = *it;
    switch( t->getType(t) )
    {
      case OPENSQ:
        result.type.array++;
        break;
    }
  }
  return result;
}

/* We do not get bitten by the identifier/typename ambiguity at this point because the
   IDENTS for symbol names are inside the DECL sub-tree for an initDtor while typenames
   are not. 
*/

static void convertDECL(pANTLR3_BASE_TREE node, SymbolTable *st)
{
TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  partitionList(children, typeToks, dtorToks, isNotDecl);

  TypeAnnotation ann;
  TypeAtom base = convertDeclSpec(typeToks.begin(), typeToks.end(), 
                                         ann, st);

  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    Decl d = convertInitDtor(base, dtor, st);
    if(ann.isTypedef)
      st->saveType(d.name, d.type);
    else
      st->saveDecl(d.name, d.type);
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
        case CHAR: case DOUBLE: case FLOAT: case INT: case LONG: case VOID: case STRUCT: case UNION:
        case SHORT: case ENUM:
          idCount = 1;    // Pretend we saw the ident
          break;
        case UNSIGNED: case AUTO:   case TYPEDEF: case EXTERN: case STATIC:
        case VOLATILE: case CONST:  case UCONST:  case URESTRICT:
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

static void convertPARAM(pANTLR3_BASE_TREE node, Decl *target, SymbolTable *st)
{
TokList children = extractChildren(node,0,-1);

  // Process: typeWrapper typeQualifier?
list<pANTLR3_BASE_TREE> typeToks, others;
  findTypeTokens(children, typeToks, others);

TypeAnnotation ann;
TypeAtom baseT = convertDeclSpec(typeToks.begin(), typeToks.end(), ann, st); 
TokList::iterator walk = others.begin();
  target->type.stars = 0;
  while(walk!=others.end())
  {
    ANTLR3_UINT32 nType = (*walk)->getType(*walk);
    if (nType==STAR)
      target->type.stars++;
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
      pANTLR3_BASE_TREE ps = findTok(others,DECLPAR) ;
      FuncType f;
      // Check if the function-pointer has no parameters (default cons above should be fine)
      if(ps!=NULL)
        f = convertParams( ps, st);
      swapReturn(f, &target->type, st, 1);
      TokList fpChildren = extractChildren(idTok, 0, -1);
      if( fpChildren.size()==1 )
      {
         pANTLR3_BASE_TREE s = *(fpChildren.begin());
         if( s->getType(s) != STAR)
           throw BrokenTree(idTok, "FPTR is all twisted and broken");
         target->name = st->anonName();
         return;
      }
      else 
      if( fpChildren.size()<2 )
      {
        throw BrokenTree(idTok, "FPTR without enough children");
      }
      pANTLR3_BASE_TREE id2Tok = *(++fpChildren.begin());
      if( id2Tok->getType(id2Tok)!=IDENT )
        throw BrokenTree(idTok, "FPTR did not contain IDENT");
      target->name = (char *)id2Tok->getText(id2Tok)->chars;
      return;
    }
    if( idTok->getType(idTok) != IDENT )
      throw BrokenTree(node, "Non-IDENT following STARs in paramter");
    target->type = baseT;
    target->name = (char*)idTok->getText(idTok)->chars;
  }
  else
  {
    target->type = baseT;     // Prototype with no name
    target->name = "";
  }
}


static void convertFUNC(SymbolTable *where, pANTLR3_BASE_TREE node)
{
pANTLR3_BASE_TREE idTok = (pANTLR3_BASE_TREE)node->getChild(node,0);
char *identifier = (char*)idTok->getText(idTok)->chars;
//TokList stmts = extractChildren((pANTLR3_BASE_TREE)node->getChild(node,1),0,-1);

  // Skip compound statement for now
TokList params;
TokList rest = extractChildren(node,2,-1);
TokList::iterator child = rest.begin();
  takeWhile( child, rest.end(), params, isParam);
TypeAnnotation ann;
TypeAtom retType = convertDeclSpec(child, rest.end(), ann, where); 


FuncType f = convertParams(NULL, where, params);
  swapReturn(f, &retType, where, 0);

Function def = Function(where);
  def.typeIdx = retType.fidx;
  where->saveFunction(identifier, def);
}



/* Synthetic nodes resulting from tree construction in the parse do not have the annotated
   state from the last preprocessor line, walk their subtree until an annotation is found.
*/
static pANTLR3_UINT8 findCustom( pANTLR3_BASE_TREE node )
{
pANTLR3_COMMON_TOKEN tok = node->getToken(node);
  if(tok->custom != NULL)
    return (pANTLR3_UINT8)tok->custom;
int count = node->getChildCount(node);
  for(int i=0; i<count; i++)
  {
    pANTLR3_UINT8 sub = findCustom((pANTLR3_BASE_TREE)node->getChild(node,i));
    if( sub!=NULL )
      return sub;
  }
  return NULL;
}

// Partial Declarations:   PartialDataType name   <- anon go in here
// Partial Records:        PartialDataType tagname

typedef pair<pANTLR3_BASE_TREE,Function*> NewRoot;    // Leftovers for second parse
static void processTopLevel(pANTLR3_BASE_TREE node, TranslationU &tu, 
                            list<NewRoot> &funcDefs, SymbolTable **curST)
{
int type  = node->getType(node);
int count = node->getChildCount(node);
pANTLR3_UINT8 lastPP = findCustom(node);
  if(lastPP!=NULL)
  {
    string s = (char*)lastPP;
    if( s.find("#line") != string::npos)
    {
      size_t pathStart = s.find('"');
      size_t pathStop  = s.find('"', pathStart+1);
      string path = s.substr(pathStart+1, pathStop-pathStart-1);
      list<string> pathComps = splitPath(path);
      pair<string,string> fnameComps = splitExt(pathComps.back());
      if( tu.headers!=NULL)
        *curST = tu.headers(&tu, path);
    }
  }
  switch(type)
  {
    case SEMI:   return;
    case DECL:   
      convertDECL(node, *curST);  // Updates table
      break;
    case FUNC:   
      convertFUNC(*curST, node);        // Updates table
      break; 
    // Pure definition, build the type in the tag namespace
    case UNION:   case STRUCT:
      convertRecord(node, *curST);  // Updates tag in SymbolTable
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
      printf("%s(%ld) ", cInCParserTokenNames[t->getType(t)], t->index);
  }
  printf("\n");
}

/* stmt must be a STATEMENT node in the AST.
*/
pANTLR3_BASE_TREE reparse(pANTLR3_BASE_TREE stmt, pANTLR3_COMMON_TOKEN_STREAM tokens, 
                          pcInCParser parser, SymbolTable *context)
{
// First token within the statement (virtual tokens have no position in the input stream)
pANTLR3_BASE_TREE node2 = (pANTLR3_BASE_TREE)stmt->getChild(stmt,0);
pANTLR3_COMMON_TOKEN tok = node2->getToken(node2);

// Move the stream to statement start and try it as a top-level declaration.
  tokens->p = tok->getTokenIndex(tok);
cInCParser_declaration_return retVal2 = parser->declaration(parser);

//PartialState unresolved;  // empty for second pass
// If it was possible to parse the STATEMENT tokens as a DECL then it is a declaration
// IFF the typenames resolve in the SymbolTable. Otherwise it is just a weird expr.
  if( retVal2.tree->getType(retVal2.tree) == DECL )
  {
    try {
      convertDECL(retVal2.tree, context);
      return retVal2.tree;
    }
    catch(BrokenTree bt)
    {
      return stmt;
    }
  }
// If it didn't parse at all as a DECL then it is definitely a STATEMENT
  return stmt;
}

TranslationU parseUnit(SymbolTable *parent, char *filename, TranslationU::PathClassifier c)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return firstPass;

/* First parse: resolve outer declarations and functions down to statement level (with statement parsetree
                in cases of typename ambiguity. */
  ip        = antlr3AsciiFileStreamNew((pANTLR3_UINT8)filename);
  // Todo: error checking for IO errors !
  lex       = cInCLexerNew(ip);
  tokens    = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  pANTLR3_VECTOR vec = tokens->getTokens(tokens);
  pANTLR3_UINT8 lastPP;
  for(int i=0; i<vec->elementsSize; i++)
  {
    pANTLR3_COMMON_TOKEN t = (pANTLR3_COMMON_TOKEN) vec->get(vec,i);
    if(t==NULL)
      continue;
    if(t->getType(t)==PREPRO)
      lastPP = t->getText(t)->chars;
    t->custom = lastPP;
  }
  parser    = cInCParserNew(tokens);
  firstPass = parser->translationUnit(parser);

/* Pass II: semantic analysis on the partial parsetree */
list<NewRoot> funcDefs;
TranslationU result = TranslationU(parent, filename,c);
SymbolTable *curST = result.table;
  /* Translation units with a single top-level declaration do not have a NIL node as parent. It only
     exists when the AST is a forest to act as a virtual root. */
  try {
    if( firstPass.tree->getType(firstPass.tree) == 0)
    {
      for(int i=0; i<firstPass.tree->getChildCount(firstPass.tree); i++)
      {
        try {
        processTopLevel((pANTLR3_BASE_TREE)firstPass.tree->getChild(firstPass.tree,i), 
                        result, funcDefs, &curST);
        }
        catch(BrokenTree bt) {
          printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
          dumpTree(bt.blame,1);
        }
      }
    }
    else
      processTopLevel(firstPass.tree, result, funcDefs, &curST);
    //unresolved.finalise(result.table);
  }
  catch(BrokenTree bt) {
    printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
    dumpTree(bt.blame,1);
  }


/* Second pass: check function statements to see if any were declarations that are legal in 
                the context of the outer symbol table + the function symbol table as it is 
                built. */

  // Dynamic overloading to prevent display of errors during speculation
  parser->pParser->rec->displayRecognitionError = dropError;
  //dumpTokenStream(tokens);

  tmplForeach(list, NewRoot, f, funcDefs)
    Function *target = f.second;
    pANTLR3_BASE_TREE body = (pANTLR3_BASE_TREE) f.first->getChild(f.first,1);
    list<pANTLR3_BASE_TREE> stmts = extractChildren(body,0,-1);
    //printf("Func: %lu stmts %s %s\n",stmts.size(), "noname", target->type->str().c_str());
    tmplForeach(list, pANTLR3_BASE_TREE, s, stmts)
      if(s->getType(s) == STATEMENT )
        reparse(s, tokens, parser, target->scope); 
      pANTLR3_COMMON_TOKEN t = s->getToken(s);
      TokList stmtToks = extractChildren(s,0,-1);
      printf("%s(%ld) ", cInCParserTokenNames[s->getType(s)], s->getToken(s)->index);
      tmplForeach( list, pANTLR3_BASE_TREE, t, stmtToks )
        printf("%s ", cInCParserTokenNames[t->getType(t)]);
      tmplEnd
      printf("\n");
    tmplEnd
  tmplEnd

  return result;
}

unsigned long hash(string s)
{
unsigned long acc=5381;        // Cheers to Bernstein for this one
  for(string::const_iterator it=s.begin(); it!=s.end(); ++it) {
    acc = (acc << 5) + acc ^ *it;
  }
  return acc;
}
/*
string dotLabel(PartialDataType p)
{
  if( p.primitive==TypeAtom::Struct || p.primitive==TypeAtom::Union)
    return ((TypeAtom)p).str() + " " + p.tag;
  return ((TypeAtom)p).str();
}

void PartialState::render(char *filename, list<DiTrip> &edges) const
{
FILE *f = fopen(filename,"wt");
  fprintf(f, "digraph{\n");

  // Project set of types onto expansion that includes pointed-to types.
set< PartialDataType > bases, nodes = deps.nodes();
  for(set< PartialDataType >::iterator it=nodes.begin(); it!=nodes.end(); ++it)
  {
    if( it->stars > 0 )
    {
      PartialDataType ptrBase = *it;
      ptrBase.stars=0;
      bases.insert(ptrBase);
    }
  }
  nodes.insert(bases.begin(), bases.end());

  // Generate GraphViz format output
  for(set< PartialDataType >::iterator it=nodes.begin(); it!=nodes.end(); ++it)
    fprintf(f, "%lu [label=\"%s\"];\n", hash(dotLabel(*it)), dotLabel(*it).c_str());
  for(list< pair<int, pair<PartialDataType,PartialDataType> > >::iterator it=edges.begin();
      it!=edges.end(); ++it)
  {
    fprintf(f, "%lu -> %lu [label=\"%d\"]; //", hash(dotLabel(it->second.first)), 
                                         hash(dotLabel(it->second.second)), it->first);
    fprintf(f, "%s  %s\n", dotLabel(it->second.first).c_str(), dotLabel(it->second.second).c_str() );
  }
  fprintf(f,"}\n");
  fclose(f);
}
*/
