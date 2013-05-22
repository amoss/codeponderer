#include "models/c-build.h"

using namespace std;

static list<Decl> convertDECL(pANTLR3_BASE_TREE node, SymbolTable *st);
static string convertPARAM(pANTLR3_BASE_TREE node, SymbolTable *st);
TypeAtom convertRecord(pANTLR3_BASE_TREE node, SymbolTable *st);

#include "models/graph.cc"

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
      case AUTO:     ann.isAuto = true;                   break;
      case EXTERN:   ann.isExtern = true;                 break;
      case STATIC:   ann.isStatic = true;                 break;
      case CONST:    result.isConst = true;               break;
      case VOLATILE: ann.isVolatile = true;               break;
      case INLINE:   ann.isInline   = true;               break;

      case TYPEDEF:  ann.isTypedef = true;                break;  
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
  if(first->getType(first) == IDENT)
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

list<Decl> record;
  tmplForeach(list, pANTLR3_BASE_TREE, f, cs)
    if( f->getType(f) == DECL )
    {
      list<Decl> partial = convertDECL(f, st);
      record.splice(record.end(), partial); 
    }
  tmplEnd
  // If there is no compound then it is not a definition so skip updating the tag
  if( record.size()>0 )
    st->saveRecord(res.tag, record);    
  return res;
}

/* Takes a declPar that is either a tail to a function-pointer or a function
   declaration. Shallow-copy is valid on FuncType objects as they do not own
   any of the DataType objects pointed to - they are the canonical instances
   from a SymbolTable. */
static TypeAtom convertParams(pANTLR3_BASE_TREE tok, SymbolTable *st)
{
TypeAtom fake;
  return fake;
/*FuncType result;
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
      // TODO: (fwd) finalisation code for building
      // result.params[i] = st->getCanon(t); 
    }
    else
    {
      PartialDataType temp;
      result.paramNames[i] = convertPARAM(parTok, &temp, unresolved);
      // TODO: (fwd) finalisation code for building
      //result.params[i] = st->getCanon(temp);
    }
    i++;
  tmplEnd
  return result;*/
}

/* On calling the result is already initialised to a copy of the base-type from the 
   declaration. 
   TODO: (fwd)
     Decouple the parsing functions from the SymbolTable entirely. Return a list of Decl
     All the injection / canonisation code moves to finalise
*/
string convertInitDtor(TypeAtom &result, pANTLR3_BASE_TREE subTree,
                       SymbolTable *st)
{
char *identifier;
TypeAtom f;
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
      identifier = (char *)id2Tok->getText(id2Tok)->chars;

      // There must be exactly one declPar
      if(numDeclPars==0)
        throw BrokenTree(subTree,"Function pointer with no parameters");

      // TODO: (fwd) this is all finalisation code, build separately
      /*
      DataType copy = *st->getCanon(result); 
      copy.stars += countTokTypes(ptrQualToks,STAR);
      f.retType = st->getCanon(result);
      result.fptr = st->getCanon(f);
      result.stars = 1;
      result.primitive = DataType::Function;
      */
      break;
    }

    // Process: IDENT 
    case IDENT:
      identifier = (char*)idTok->getText(idTok)->chars;
      if(numDeclPars==1)
      {
        /* TODO: (fwd) canonisation is building move to finalisation
        result.stars += countTokTypes(ptrQualToks,STAR);
        f.retType = st->getCanon(result);
        result.primitive = DataType::Function;  // star
        result.fptr = st->getCanon(f);
        result.stars = 0;
        */
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
   TODO: (fwd)
     Decouple the parsing functions from the SymbolTable entirely. Return a list of Decl
     All the injection / canonisation code moves to finalise
*/

static list<Decl> convertDECL(pANTLR3_BASE_TREE node, SymbolTable *st)
{
list<Decl> result;
TokList typeToks, dtorToks, children = extractChildren(node,0,-1);
  partitionList(children, typeToks, dtorToks, isNotDecl);

  TypeAnnotation ann;
  TypeAtom base = convertDeclSpec(typeToks.begin(), typeToks.end(), 
                                         ann, st);
  //base.node = node;

  tmplForeach(list,pANTLR3_BASE_TREE,dtor,dtorToks)
    TypeAtom decltype = base;
    string name = convertInitDtor(decltype, dtor, st);
    result.push_back( Decl(name,decltype) );
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

static string convertPARAM(pANTLR3_BASE_TREE node, TypeAtom *target,
                           SymbolTable *st)
{
TokList children = extractChildren(node,0,-1);

  // Process: typeWrapper typeQualifier?
list<pANTLR3_BASE_TREE> typeToks, others;
  findTypeTokens(children, typeToks, others);

TypeAnnotation ann;
TypeAtom base = convertDeclSpec(typeToks.begin(), typeToks.end(), ann, st); 
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
      // TODO: (fwd) all building/canonc code to finalisation
      /*
      pANTLR3_BASE_TREE ps = findTok(others,DECLPAR) ;
      FuncType f;
      // Check if the function-pointer has no parameters (default cons above should be fine)
      if(ps!=NULL)
        f = convertParams( ps, unresolved);
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
      */
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


static Function *convertFUNC(TranslationU &where, pANTLR3_BASE_TREE node)
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
TypeAtom retType = convertDeclSpec(child, rest.end(), ann, where.table); 


  // Once upon a time these parameters were parsed as declarations, back when the world was 
  // young and the token stream was flat. But it because necessary to indicate dtor boundaries
  // so that IDENTs could be resolved into symbol names and typedef'd names. Hence the 
  // slightly ugly construction here.
/*typedef pair<DataType,string> Binding;
list<Binding> pBinding;
  tmplForeach( list, pANTLR3_BASE_TREE, p, params)
    PartialDataType temp;
    string name = convertPARAM(p, &temp, unresolved);
    pBinding.push_back( pair<PartialDataType,string>(temp,name) );
  tmplEnd*/

  return new Function(where.table);
  // Parse the parameters and build the FuncType
/* TODO:
      Cannot fix functions until after the type canonicalisation is finished
FuncType f;
  f.retType = where.table->getCanon(retType);
  f.nParams = pBinding.size();
  f.params     = new const DataType*[f.nParams];
  f.paramNames = new string[f.nParams];
  int idx=0;
  tmplForeach(list, Binding, both, pBinding)
    f.params[idx]     = where.table->getCanon(both.first);
    f.paramNames[idx++] = both.second;
  tmplEnd

Function *def = new Function(f,where.table);
  where.table->functions[identifier] = def;
  return def;
  */
}




// Partial Declarations:   PartialDataType name   <- anon go in here
// Partial Records:        PartialDataType tagname

typedef pair<pANTLR3_BASE_TREE,Function*> NewRoot;    // Leftovers for second parse
static void processTopLevel(pANTLR3_BASE_TREE node, TranslationU &tu, 
                            list<NewRoot> &funcDefs)
{
int type  = node->getType(node);
int count = node->getChildCount(node);
  switch(type)
  {
    case PREPRO: return;
    case SEMI:   return;
    case DECL:   
    {
      list<Decl> decls = convertDECL(node, tu.table);
      tmplForeach(list, Decl, d, decls)
        tu.table->symbols[d.name] = d.type;
      tmplEnd
      break;
    }
    case FUNC:   
    {
      Function *f = convertFUNC(tu, node);
      //funcDefs.push_back( NewRoot(node,f) );
      break; 
    }
    // Pure definition, build the type in the tag namespace
    case UNION: 
    case STRUCT:
      convertRecord(node, tu.table);  // Updates tag in SymbolTable
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

TranslationU parseUnit(char *filename)
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
  parser    = cInCParserNew(tokens);
  firstPass = parser->translationUnit(parser);

/* Pass II: semantic analysis on the partial parsetree */
list<NewRoot> funcDefs;
TranslationU result;
  /* Translation units with a single top-level declaration do not have a NIL node as parent. It only
     exists when the AST is a forest to act as a virtual root. */
  try {
    if( firstPass.tree->getType(firstPass.tree) == 0)
    {
      for(int i=0; i<firstPass.tree->getChildCount(firstPass.tree); i++)
      {
        try {
        processTopLevel((pANTLR3_BASE_TREE)firstPass.tree->getChild(firstPass.tree,i), 
                        result, funcDefs);
        }
        catch(BrokenTree bt) {
          printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
          dumpTree(bt.blame,1);
        }
      }
    }
    else
      processTopLevel(firstPass.tree, result, funcDefs);
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

/*const DataType *PartialDataType::makeCanon(SymbolTable *target, SymbolTable *namesp)
{
    DataType building = (TypeAtom)*this;
    building.nFields = namesp->symbols.size();
    building.fields  = new const DataType*[building.nFields];
    int idx=0;
    for(list<Decl>::iterator f=fields.begin(); f!=fields.end(); ++f)
      building.fields[idx++] = namesp->symbols[f->name];
    return target->getCanon(building);
}

string PartialDataType::str() const
{
stringstream res;
  res << ((TypeAtom)*this).str();
  if(primitive==TypeAtom::Struct || primitive==TypeAtom::Union)
  {
    res << " " << tag << " ";
    res << "{ ";
    for(list<Decl>::const_iterator it = fields.begin(); it!=fields.end(); ++it)
    {
      res << it->type.str();
      res << "; ";
    }
    res << "}";
  }
  return res.str();
}*/

/* Build data-types bottom-up.
   Resolve tag and typedef names, register in SymbolTable and canonise every part 
   of the type. This operation does not need to be atomic, if sub-parts succeed and
   the whole fails then it will be repeated later - the canonical values of the 
   sub-parts will not change inbetween.

  More TODO (fwd):
   name parameter implies this is a decl.   FIXED
   no way to handle puredefs in below       FIXED
   no error cases for typedef / tag resolution errors
*/
/*
bool PartialDataType::finalise(SymbolTable *st, std::string name, TypeAnnotation ann,
                               list<string> &waiting)
{
  if(partial)
    return false;

  if(primitive==DataType::Struct || primitive==DataType::Union)
  {
    SymbolTable *namesp = new SymbolTable(st);
    for(list<Decl>::iterator f=fields.begin(); f!=fields.end(); ++f)
      if(!f->type.finalise(namesp, f->name, f->ann, waiting))
      {
        delete namesp;
        printf("Pushing %s to waiting\n", tag.c_str());
        waiting.push_back(tag);
        return false;
      }
    if(fields.size() == 0) {
      if(name.size()==0)
      {
        printf("Forward ref %s\n", tag.c_str());
        waiting.push_back(tag);
        return true;
      }
      else {
        if(tag.length()==0)
          printf("Anonymous!\n");
        else
          printf("TAG %s %zu\n", tag.c_str(), waiting.size());
        const DataType *record = st->lookupTag(tag);
        if( record==NULL )
        {
          tmplForeach(list, string, t, waiting)
            printf("Iterate?\n");
            printf("%s\n",t.c_str());
            if( t==tag )
              return false;
          tmplEnd
          throw BrokenTree(node, "Unknown tag used");
        }
        printf("Lookup tag %s -> %p %s\n", tag.c_str(), record, record->str().c_str());
      }
    }
    const DataType *c = makeCanon(st, namesp);
    if(name.length()>0)
      st->symbols[name] = c;
    else
      st->tags[tag] = c;
    return true;

  }


const DataType *c = st->getCanon(*this) ;
  if(ann.isTypedef)
    st->typedefs[name] = c;
  else
    st->symbols[name] = c;
  return true;
}*/

        /* TODO: (fwd)
        {
          // Decide if the tag has been resolved yet, or is a forward-reference
          printf("Processed %s\n", result.tag.c_str());
          if( result.nFields==0 )   // Using a tag with no compound
          {
            const DataType *tagDef = st->lookupTag(result.tag);
            if(tagDef==NULL)
            {
              // Use of a tagname that is a forward-reference
              if( !unresolved.findTag(result.tag) )
                throw BrokenTree(tok,"Unknown tag used");
              result.partial = true;
              return result;
            }
            result = *tagDef;
            // TODO: If the struct was initialised by a forward-reference then we cannot pass it by value
            //       as no value has been constructed. Using the const DataType* would break the 
            //       interface in use here...
          }
          else                      // Defining and using a tag
            st->tags[result.tag] = st->getCanon(result);
        }
        */


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


// false = definite difference, true = maybe equal
bool similar(PartialDataType const &a, PartialDataType const &b)
{
  if(a.primitive!=b.primitive)
    return false;
  if(a.stars!=b.stars)
    return false;
  if(a.array!=b.array)
    return false;
  if(a.fields.size() != b.fields.size())
    return false;
  return true;
}

list<set<PartialDataType> > split(set<PartialDataType> const &orig)
{
list<set<PartialDataType> > buckets;
  tmplForeachConst(set, PartialDataType, t, orig)
    bool stored = false;
    tmplForeach(list, set<PartialDataType>, b, buckets)
      if( similar(t, *b.begin() ) )
      {
        b.insert(t);
        stored = true;
        break;
      }
    tmplEnd
    if(!stored)
    {
      set<PartialDataType> newBucket;
      newBucket.insert(t);
      buckets.push_back(newBucket);
    }
  tmplEnd
  return buckets;
}
*/

/*
void PartialState::finalise(SymbolTable *st)
{
set<PartialDataType> allNode = deps.nodes();
list<set<PartialDataType> > buckets = split(allNode);
  tmplForeach(list, set<PartialDataType>, b, buckets)
    printf("Bucket(%u) ",b.size());
    tmplForeachConst(set, PartialDataType, v, b)
      printf("%s ",v.str().c_str());
    tmplEnd
    printf("\n");
  tmplEnd


list<DiGraph<PartialDataType,int>::Triple> edges = deps.edges();
  render((char*)"fwds.dot", edges);
  DiGraph<PartialDataType,int> g2 = deps.flip();
  edges = g2.edges();
  render((char*)"bwds.dot", edges);
  set<PartialDataType> sources = deps.sources();
  list<PartialDataType> sorted = deps.topSort(sources);
  tmplForeach(list, PartialDataType, p, sorted)
    printf("tops: %s\n", p.str().c_str());
  tmplEnd
  sources = g2.sources();
  sorted = g2.topSort(sources);
  tmplForeach(list, PartialDataType, p, sorted)
    printf("bwdtops: %s\n", p.str().c_str());
  tmplEnd
  for(set<PartialDataType>::iterator it=sources.begin(); it!=sources.end(); ++it)
  {
    printf("Source: %s\n", it->str().c_str());
    set<PartialDataType> front = deps.reachable(*it);
    for(set<PartialDataType>::iterator r=front.begin(); r!=front.end(); ++r)
      printf("Reaches: %s\n", r->str().c_str());
  }
  set<PartialDataType> sinks = deps.sinks();
  for(set<PartialDataType>::iterator it=sinks.begin(); it!=sinks.end(); ++it)
    printf("Sink: %s\n", it->str().c_str());
list<Decl>::iterator it = decls.begin();
  for(; it!=decls.end() ;)
  {
    printf("Finalise decl: %s %s\n", it->type.str().c_str(), it->name.c_str());
    if(it->type.finalise(st, it->name, it->ann, waitingTags))
    {
      printf("Finalised %s\n", it->name.c_str());
      it = decls.erase(it);
    }
    else
    {
      printf("Skipping unresolved %s\n", it->name.c_str());
      ++it;
    }
  }
}

void PartialState::insert(PartialDataType p)
{
  int idx=0;
  // If there is a forward-ref to a record type then it must be updated
  deps.forceUpdate(p);

  // Fold pointer types onto a common base
  PartialDataType base=p;
  if(p.stars>0) 
  {
    base.stars=0;
    deps.add(p, base, -1);
  }
  for(list<Decl>::iterator it = p.fields.begin(); it!=p.fields.end(); ++it)
  {
    // Same pointer fold for rhs of edge tuple
    PartialDataType copy = it->type;
    if(copy.stars>0)
    {
      copy.stars = 0;
      deps.add(it->type, copy, -1);
    }
    deps.add(p, it->type, idx++);
  }
}*/
