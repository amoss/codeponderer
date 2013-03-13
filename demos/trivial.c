#include "cInCLexer.h"
#include "cInCParser.h"

char testip[] = "float x;";

int main(int argc, char **argv)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
  ip = antlr3NewAsciiStringInPlaceStream(testip, strlen(testip), NULL);
  lex = cInCLexerNew(ip);
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  parser = cInCParserNew(tokens);
  parser->translationUnit(parser);

}
