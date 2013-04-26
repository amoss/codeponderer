#include "models/c-build.h"

using namespace std;
/* Check one top-level (external) declaration in the translation unit and generate a Decl or
   FuncDef as appropriate. */
static void processTopLevel(pANTLR3_BASE_TREE node, list<Decl*> &globals, list<FuncDef*> &functions )
{
int type  = node->getType(node);
int count = node->getChildCount(node);
  switch(type)
  {
    case PREPRO: return;
    case SEMI:   return;
    case DECL:
      Decl::parse(node,globals);
      break;
    case FUNC:
      {
        FuncDef *f = new FuncDef;
        f->parse(node);
        functions.push_back(f);
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

DataType typeConvert(Type *orig)
{
DataType result;
  switch(orig->primType)
  {
    case DOUBLE:
      result.primitive = DataType::Double;
      break;
    case FLOAT:
      result.primitive = DataType::Float;
      break;
    case CHAR:
      result.primitive = DataType::Char;
      break;
    case SHORT:
      result.primitive = DataType::Short;
      break;
    case LONG:
      result.primitive = DataType::Long;
      break;
    case INT:
      result.primitive = DataType::Int;
      break;
  }
  result.isUnsigned = orig->isUnsigned;
  result.stars      = orig->stars;
  return result;
}

bool testConvertDecl(Decl *d, SymbolTable *target)
{
  if( d->type.primType != TYPEDEF )
    return true;
  map<string,DataType*>::iterator it = target->typedefs.find(d->type.typedefName);
  if( it==target->typedefs.end() )
    return false;
  return true;
}

void convertDecls(list<Decl*> &src, SymbolTable *target)
{
  tmplForeach(list, Decl*, d, src)
    // Resolve the typename with a lookup in the typedef namespace
    if(d->type.primType == TYPEDEF)
    {
      map<string,DataType*>::iterator it = target->typedefs.find(d->type.typedefName);
      if( it==target->typedefs.end() )
        printf("Unknown type! %s\n", d->type.typedefName);
      else
        target->symbols[d->identifier] = it->second;
    }
    else 
    {
      // Convert representation and lookup canonical DataType by value.
      DataType *canon = target->getCanon( typeConvert(&d->type) );
      if(d->type.isTypedef )
        target->typedefs[d->identifier] = canon;
      else
        target->symbols[d->identifier] = canon;
    }
  tmplEnd
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
list<Decl*> globals;
list<FuncDef*> functions;
  /* Translation units with a single top-level declaration do not have a NIL node as parent. It only
     exists when the AST is a forest to act as a virtual root. */
  try {
    if( firstPass.tree->getType(firstPass.tree) == 0)
    {
      for(int i=0; i<firstPass.tree->getChildCount(firstPass.tree); i++)
          processTopLevel((pANTLR3_BASE_TREE)firstPass.tree->getChild(firstPass.tree,i), globals, functions);
    }
    else
      processTopLevel(firstPass.tree, globals, functions);
  }
  catch(BrokenTree bt) {
    printf("ERROR(%u): %s\n", bt.blame->getLine(bt.blame), bt.explain);
    dumpTree(bt.blame,1);
  }

// Build the top-level symbol table 
  SymbolTable *st = new SymbolTable;
  convertDecls(globals,st);
  st->dump();

/* Second pass: check function statements to see if any were declarations that are legal in the context
                of the outer symbol table + the function symbol table as it is built. */
  // Dynamic overloading to prevent display of errors during speculation
  parser->pParser->rec->displayRecognitionError = dropError;

  pANTLR3_VECTOR vec = tokens->getTokens(tokens);
  printf("%u\n", vec->elementsSize);
  for(int i=0; i<vec->elementsSize; i++)
  {
    pANTLR3_COMMON_TOKEN t = (pANTLR3_COMMON_TOKEN) vec->get(vec,i);
    if(t!=NULL)
      printf("%s(%u) ", cInCParserTokenNames[t->getType(t)], t->index);
  }

  tmplForeach(list, FuncDef*, f, functions)
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
      //printf("index %u\n", tok->index);
      //printf("token index %u\n", tok->getTokenIndex(tok));
      //printf("start index %u\n", tok->getStartIndex(tok));
      //printf("stop index %u\n", tok->getStopIndex(tok));
      tokens->p = tok->getTokenIndex(tok);
      //printf("Checking index %u\n",tokens->p);
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
            if(!testConvertDecl(v,st))      // st should be const during this loop
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
}
