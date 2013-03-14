extern "C" {
#include "cInCLexer.h"
#include "cInCParser.h"
}
#include<list>
using namespace std;
char testip[] = "float x;\nstatic int z;\nchar text[64] = \"hello\"; int bob(char x, char *harry) { stuff  { inside } }";

class Decl
{
public:
  char *identifier;
};

class Func
{
public:
  char *identifier;
};

class TranslationU
{
public:
  list<Decl*> globals;
  list<Func*> functions;
};

void *processTree(pANTLR3_BASE_TREE node, int depth)
{
int count = node->getChildCount(node); 
int type  = node->getType(node);
  switch(type)
  {
    case DECL:
      {
        Decl *d = new Decl;
        pANTLR3_BASE_TREE id = (pANTLR3_BASE_TREE)node->getChild(node,0);
        d->identifier = (char*)id->getText(id)->chars;
        return d;
      }
    case FUNC:
      break;
    default:
      for(int i=0; i<depth; i++)
        printf("  ");

      printf("Type %u Children %u ", node->getType(node), count);
      if(node->getText(node)!=NULL)
        printf("%s\n", node->getText(node)->chars);
      else
        printf("empty\n");
      for(int i=0; i<count; i++)
        processTree((pANTLR3_BASE_TREE)node->getChild(node,i), depth+1);
      break;
  }
}

int main(int argc, char **argv)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return retVal;

  ip = antlr3NewAsciiStringInPlaceStream((uint8_t*)testip, strlen(testip), NULL);
  lex = cInCLexerNew(ip);
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  parser = cInCParserNew(tokens);
  retVal = parser->translationUnit(parser);
  processTree(retVal.tree,0);

}
