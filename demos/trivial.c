#include "cInCLexer.h"
#include "cInCParser.h"

char testip[] = "float x;";

void processTree(pANTLR3_BASE_TREE node, int depth)
{
int count = node->getChildCount(node); 
  for(int i=0; i<depth; i++)
    printf("  ");

  printf("Type %u Children %u\n", node->getType(node), count);
  for(int i=0; i<count; i++)
    processTree(node->getChild(node,i), depth+1);
}

int main(int argc, char **argv)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return retVal;

  ip = antlr3NewAsciiStringInPlaceStream(testip, strlen(testip), NULL);
  lex = cInCLexerNew(ip);
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  parser = cInCParserNew(tokens);
  retVal = parser->translationUnit(parser);
  processTree(retVal.tree,0);

}
