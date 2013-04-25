using namespace std;
#include "models/c.h"
char testip[] = "float x;\nstatic int z;\nchar text[64] = \"hello\"; static int bob(char x, char *harry) { stuff  { inside } }";

/* Perform rangecheck on getChild before checking the type. Uses -1 as a sentinel 
   for the type of an out of range child to simplify calling contexts.
*/
int getChildType(pANTLR3_BASE_TREE parent, int idx)
{
  int count = parent->getChildCount(parent);
  if( idx<0 || idx>=count )
    return -1;
  pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)parent->getChild(parent,idx);
  return child->getType(child);
}

extern pANTLR3_UINT8   cInCParserTokenNames[];
void dropError(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 * tokenNames)
{
}

int main(int argc, char **argv)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return retVal;
cInCParser_declaration_return retVal2;

  if( argc==1 )
    ip = antlr3NewAsciiStringInPlaceStream((uint8_t*)testip, strlen(testip), NULL);
  else
    ip = antlr3AsciiFileStreamNew((pANTLR3_UINT8)argv[1]);
  // Todo: error checking for IO errors !
  lex = cInCLexerNew(ip);
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  printf("%u\n",tokens->p);
  parser = cInCParserNew(tokens);
  retVal = parser->translationUnit(parser);

TranslationU model = TranslationU(retVal.tree);
  model.buildSymbolTable();
  model.dump();
  dumpTree(retVal.tree,0);

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

  tmplForeach(list, FuncDef*, f, model.functions)
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
        // True iff IDENTs were valid typenames
        dumpTree(retVal2.tree,0);
      }
      else 
      {
        // Must be a statement if they were not.
        dumpTree(s,0);
      }
    tmplEnd
  tmplEnd
  
}
