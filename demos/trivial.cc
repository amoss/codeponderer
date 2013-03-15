extern "C" {
#include "cInCLexer.h"
#include "cInCParser.h"
}
#include<list>
#include<string>
using namespace std;
char testip[] = "float x;\nstatic int z;\nchar text[64] = \"hello\"; int bob(char x, char *harry) { stuff  { inside } }";

class Decl
{
public:
  char *identifier;
  bool typeStatic, typeExtern, typeTypedef, typeAuto, typeRegister;
  int  primType;    // -1 for things that are not
  Decl() :
    primType(-1)
  {
  }
  string typeStr()
  {
    return "hello";
  }
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
  TranslationU(pANTLR3_BASE_TREE root);
  void processTopLevel(pANTLR3_BASE_TREE node);
  void dump();
};

TranslationU::TranslationU(pANTLR3_BASE_TREE root)
{
  if( root->getType(root)==0 ) 
  {
    int count = root->getChildCount(root); 
    for(int i=0; i<count; i++)
      processTopLevel((pANTLR3_BASE_TREE)root->getChild(root,i));
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
      {
        Decl *d = new Decl;
        pANTLR3_BASE_TREE id = (pANTLR3_BASE_TREE)node->getChild(node,0);
        d->identifier = (char*)id->getText(id)->chars;
        for(int i=1; i<count; i++)
        {
          pANTLR3_BASE_TREE tok = (pANTLR3_BASE_TREE)node->getChild(node,i);
          switch(tok->getType(tok))
          {
            case FLOAT:
            case CHAR:
            case INT:
            case LONG:
              d->primType = tok->getType(tok);
              break;
            case AUTO:
              d->typeAuto = true;
              break;
            case TYPEDEF:
              d->typeTypedef = true;
              break;
            case EXTERN:
              d->typeExtern = true;
              break;
            case STATIC:
              d->typeStatic = true;
              break;
            default:
              printf("-->%s %d\n", (char*)tok->getText(tok)->chars, tok->getType(tok));
              break;
          }
        }
        globals.push_back(d);
      }
    case FUNC:
      break;
    default:
      printf("Unknown Type %u Children %u ", type, count);
      if(node->getText(node)!=NULL)
        printf("%s\n", node->getText(node)->chars);
      else
        printf("empty\n");
      break;
  }
}

// C++11 is scary and both "differently" supported and "differently" able.
// Also upgrading g++ on the knackered old version of Ubuntu that I use can wait until another day.
#define foreach(T,V,C) for(T::iterator V = C.begin(); V!=C.end(); ++V)
// This is definitely a bit messy... but it works :)
#define tmplForeach(Tmpl,ElemT,V,C) { Tmpl<ElemT>::iterator V##It; ElemT V; for(V##It = C.begin(),V=*V##It; V##It!=C.end(); ++V##It,V=*V##It)
#define tmplEnd }

void TranslationU::dump()
{
  tmplForeach(list,Decl*,decl,globals)  
    printf("Declation: %s <- %s\n", decl->identifier, decl->typeStr().c_str());
  tmplEnd
  tmplForeach(list,Func*,f,functions)  
    printf("Function: %s <- %s\n", f->identifier, "type");
  tmplEnd
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

TranslationU model = TranslationU(retVal.tree);
  model.dump();

}
