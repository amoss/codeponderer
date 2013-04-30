#include "models/c-build.h"

using namespace std;

static list<string> convertDECL(SymbolTable *st, pANTLR3_BASE_TREE node);
static string convertPARAM(SymbolTable *st, pANTLR3_BASE_TREE node, DataType *target);
DataType convertRecord(SymbolTable *st, pANTLR3_BASE_TREE node, string &tag);

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
DataType convertDeclSpec(SymbolTable *st, TokList::iterator start, TokList::iterator end,
                         TypeAnnotation &ann)
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
      case AUTO:     ann.isAuto = true;                   break;
      case EXTERN:   ann.isExtern = true;                 break;
      case STATIC:   ann.isStatic = true;                 break;
      case CONST:    result.isConst = true;               break;
      case VOLATILE: ann.isVolatile = true;               break;
      case INLINE:   ann.isInline   = true;               break;

      case TYPEDEF:  ann.isTypedef = true;                break;  
      case ELLIPSIS:                                      break;

      case IDENT:
        {
          const DataType *theType = st->lookupTypedef((char*)tok->getText(tok)->chars);
          if(theType==NULL)
            throw BrokenTree(tok, "Unknown type");
          else
            return *theType;
        }
      case UNION:
      case STRUCT:
      {
        string tag;
        result = convertRecord(st, tok,tag);
        if( tag.length() > 0 )
        {
          if( result.nFields==0 )   // Using a tag with no compound
          {
            const DataType *tagDef = st->lookupTag(tag);
            if(tagDef==NULL)
              throw BrokenTree(tok,"Unknown tag used");
            result = *tagDef;
          }
          else                      // Defining and using a tag
            st->tags[tag] = st->getCanon(result);
        }
        // With a compound but no tag convertRecord has already built the type
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
DataType convertRecord(SymbolTable *st, pANTLR3_BASE_TREE node, string &tag)
{
DataType res;
  if( node->getType(node)==STRUCT )
    res.primitive = DataType::Struct;
  else 
    res.primitive = DataType::Union;

  // If there is an IDENT? then process it separately from the DECL loop
TokList cs = extractChildren(node,0,-1);
pANTLR3_BASE_TREE first = *cs.begin();
  if(first->getType(first) == IDENT)
  {
    cs.pop_front();
    tag = (char*)first->getText(first)->chars;
  }

// When we call convertDECL it needs a SymbolTable to record the basetype+initdecl for each
// declarator in the declaration. We can't use the real TU table as these declarations exist
// in the private namespace of the record. To build the DataType that represents the record
// we can drop the names but we need the declaration DataTypes in the correct order...
// Type are canonical in private namespace... 
list<string> order;
  res.namesp = new SymbolTable(st);
  tmplForeach(list, pANTLR3_BASE_TREE, f, cs)
    if( f->getType(f) == DECL )
    {
      list<string> partial = convertDECL(res.namesp,f);
      order.splice(order.end(), partial); 
    }
  tmplEnd
  res.nFields = res.namesp->symbols.size();
  res.fields  = new const DataType*[res.nFields];
  int idx = 0;
  tmplForeach(list, string, name, order)
    res.fields[idx++] = res.namesp->symbols[name];
  tmplEnd
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
    // Note: there is an unused ELLIPSIS case inside convertDeclSpec that is called from
    //       convertPARAM. It could simplify this to move it there.
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
  TokList::iterator it = dtorToks.begin();
  while(++it != dtorToks.end())
  {
    pANTLR3_BASE_TREE t = *it;
    switch( t->getType(t) )
    {
      case OPENSQ:
        result.array++;
        break;
    }
  }
  return identifier;
}

/* We do not get bitten by the identifier/typename ambiguity at this point because the
   IDENTS for symbol names are inside the DECL sub-tree for an initDtor while typenames
   are not. 
   Because a DECL can produce several declarations this function writes the results into
   the supplied SymbolTable, the names are returned to preserve the order of the declarations
   (e.g. if we are building a record where we need to know the layout).
*/
static list<string> convertDECL(SymbolTable *st, pANTLR3_BASE_TREE node)
{
list<string> result;
TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  partitionList(children, typeToks, dtorToks, isNotDecl);

  // Typedef IDENTs are converted during the convertDeclSpec call.
  TypeAnnotation ann;
  DataType base = convertDeclSpec(st, typeToks.begin(), typeToks.end(), ann); 
  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    DataType decl = base;
    string name = convertInitDtor(st, decl, dtor);
    const DataType *c = st->getCanon(decl) ;
    if(ann.isTypedef)
      st->typedefs[name] = c;
    else
      st->symbols[name] = c;
    result.push_back(name);
  tmplEnd
  return result;
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

TypeAnnotation ann;
DataType base = convertDeclSpec(st, typeToks.begin(), typeToks.end(), ann); 
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
      pANTLR3_BASE_TREE ps = findTok(others,DECLPAR) ;
      FuncType f;
      // Check if the function-pointer has no parameters (default cons above should be fine)
      if(ps!=NULL)
        f = convertParams( st, ps);
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


static void convertFUNC(TranslationU &where, pANTLR3_BASE_TREE node)
{
/*
  FuncDef *f = new FuncDef;
  f->parse(node);
  functions.push_back(f);

pANTLR3_BASE_TREE idTok = (pANTLR3_BASE_TREE)node->getChild(node,0);
  identifier = (char*)idTok->getText(idTok)->chars;
TokList stmts = extractChildren((pANTLR3_BASE_TREE)node->getChild(node,1),0,-1);
  printf("%u statements in %s\n", stmts.size(), identifier);
  tmplForeach( list, pANTLR3_BASE_TREE, s, stmts)
    stmtNodes.push_back(s);
    pANTLR3_COMMON_TOKEN t = s->getToken(s);
    TokList stmtToks = extractChildren(s,0,-1);
    printf("%s(%u) ", cInCParserTokenNames[s->getType(s)], s->getToken(s)->index);
    tmplForeach( list, pANTLR3_BASE_TREE, t, stmtToks )
      printf("%s ", cInCParserTokenNames[t->getType(t)]);
    tmplEnd
    printf("\n");
  tmplEnd
  // Skip compound statement for now
TokList params;
TokList rest = extractChildren(node,2,-1);
TokList::iterator child = rest.begin();
  //REWRITE ---> moved into build takeWhile( child, rest.end(), params, isParam);
  retType.parse(child, rest.end());
  // Once upon a time these parameters were parsed as declarations, back when the world was young and
  // the token stream was flat. But it because necessary to indicate dtor boundaries so that IDENTs
  // could be resolved into symbol names and typedef'd names. Hence the slightly ugly construction here 
  tmplForeach( list, pANTLR3_BASE_TREE, p, params)
    Type dummy;
    char *name = parseParam(p, &dummy);
    Decl *d = new Decl(dummy);
    d->identifier = name;
    args.push_back(d);
  tmplEnd
  */
}






static void processTopLevel(pANTLR3_BASE_TREE node, TranslationU &tu)
{
int type  = node->getType(node);
int count = node->getChildCount(node);
  switch(type)
  {
    case PREPRO: return;
    case SEMI:   return;
    case DECL:   convertDECL(tu.table, node);         break;
    case FUNC:   convertFUNC(tu, node);               break; 
    // Pure definition, build the type in the tag namespace
    case UNION: 
    case STRUCT:
    {
      string tag;
      DataType r = convertRecord(tu.table, node,tag);
      tu.table->tags[tag] = tu.table->getCanon(r);
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
      {
        try {
        processTopLevel((pANTLR3_BASE_TREE)firstPass.tree->getChild(firstPass.tree,i), result);
        }
        catch(BrokenTree bt) {
          printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
          dumpTree(bt.blame,1);
        }
      }
    }
    else
      processTopLevel(firstPass.tree, result);
  }
  catch(BrokenTree bt) {
    printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
    dumpTree(bt.blame,1);
  }

  //dumpTree(firstPass.tree,0);


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
