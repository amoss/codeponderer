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


/*class Func
{
public:
  char *identifier;
  Decl retType;
  list<Decl*> params;
};
*/

class TranslationU
{
public:
  list<Decl*> globals;
  //list<Func*> functions;
  TranslationU(pANTLR3_BASE_TREE root);
  void processTopLevel(pANTLR3_BASE_TREE node);
  void dump();
};

TranslationU::TranslationU(pANTLR3_BASE_TREE root)
{
  if( root->getType(root)==0 ) 
  {
    TokList tops = extractChildren(root, 0, -1);
    tmplForeach(list, pANTLR3_BASE_TREE, tok, tops)
      processTopLevel(tok);
    tmplEnd
  }
  else
    processTopLevel(root);
}

void TranslationU::processTopLevel(pANTLR3_BASE_TREE node)
{
int type  = node->getType(node);
int count = node->getChildCount(node);
  switch(type)
  {
    case DECL:
      Decl::parse(node,globals);
      break;
    /*case FUNC:
      {
        Func *f = new Func;
        pANTLR3_BASE_TREE id = (pANTLR3_BASE_TREE)node->getChild(node,0);
        f->identifier = (char*)id->getText(id)->chars;
        list<pANTLR3_BASE_TREE> rest = extractChildren(node,1,-1);
        list<pANTLR3_BASE_TREE>::iterator it=rest.begin();
        for(; it!=rest.end(); ++it) 
        {
          pANTLR3_BASE_TREE tok = *it;
          if( tok->getType(tok)==PARAM )
          {
            Decl *p = new Decl;
            p->parseParam(tok);
            f->params.push_back(p);
          }
          else 
            break;
        }
        if(it==rest.end()) {
          printf("Malformed function definition - no body\n");
          dumpTree(node,0);
          return;
        }
        ++it;
        f->retType.parseSpecifiers(it,rest.end());
        functions.push_back(f);
      }
      break;*/
    case SEMI:
      break;
    default:
      printf("Unknown Type %u Children %u ", type, count);
      dumpTree(node,1);
      break;
  }
}

void TranslationU::dump()
{
  tmplForeach(list,Decl*,decl,globals)  
    printf("Declation: %s is %s\n", decl->identifier, decl->type.str().c_str());
  tmplEnd
  /*tmplForeach(list,Func*,f,functions)  
    printf("Function: %s is ", f->identifier);
    printf("%s <- ", f->retType.typeStr().c_str());
    tmplForeach(list,Decl*,p,f->params)
      printf("%s ", p->typeStr().c_str());
      printf("%s  ", p->identifier);
    tmplEnd
    printf("\n");
  tmplEnd*/
}

int main(int argc, char **argv)
{
pANTLR3_INPUT_STREAM ip;
pANTLR3_COMMON_TOKEN_STREAM tokens;
pcInCLexer lex;
pcInCParser parser;
cInCParser_translationUnit_return retVal;

  if( argc==1 )
    ip = antlr3NewAsciiStringInPlaceStream((uint8_t*)testip, strlen(testip), NULL);
  else
    ip = antlr3AsciiFileStreamNew((pANTLR3_UINT8)argv[1]);
  lex = cInCLexerNew(ip);
  tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lex));
  parser = cInCParserNew(tokens);
  retVal = parser->translationUnit(parser);

TranslationU model = TranslationU(retVal.tree);
  model.dump();
  //dumpTree(retVal.tree,0);

}
