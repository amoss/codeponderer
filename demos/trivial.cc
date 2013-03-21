extern "C" {
#include "cInCLexer.h"
#include "cInCParser.h"
}
#include<list>
#include<string>
#include<sstream>
using namespace std;
char testip[] = "float x;\nstatic int z;\nchar text[64] = \"hello\"; static int bob(char x, char *harry) { stuff  { inside } }";


//////////////////// Heading into some sort of misc.cc pile //////////////////////

// Implements the python str.join function on lists of strings. Awkward to inline without
// a helper function because the first iteration doesn't start with a separator so it needs
// to be lifted from the loop.
string joinStrings(list<string> &strs, char separator)
{
stringstream res;
list<string>::iterator it = strs.begin();
  if(it!=strs.end())
  {
    res << *it;
    ++it;
  }
  while(it!=strs.end())
  {
    res << separator << *it;
    ++it;
  }
  return res.str();
}


// C++11 is scary and both "differently" supported and "differently" able.
// Also upgrading g++ on the knackered old version of Ubuntu that I use can wait until another day.
#define foreach(T,V,C) for(T::iterator V = C.begin(); V!=C.end(); ++V)
// This is definitely a bit messy... but it works :)
#define tmplForeach(Tmpl,ElemT,V,C) { Tmpl<ElemT>::iterator V##It; ElemT V; for(V##It = C.begin(),V=*V##It; V##It!=C.end(); ++V##It,V=*V##It) {
#define tmplEnd } }

/* Utility function: extract a slice of the children into a list */
list<pANTLR3_BASE_TREE> extractChildren(pANTLR3_BASE_TREE node, int lo, int hi)
{
list<pANTLR3_BASE_TREE> result;
int count = node->getChildCount(node);
  if(hi==-1 || hi >=count)
    hi = count-1;
  for(int i=lo; i<=hi; i++)
    result.push_back( (pANTLR3_BASE_TREE)node->getChild(node,i) );
  return result;
}

void dumpTree(pANTLR3_BASE_TREE node, int depth)
{
int count = node->getChildCount(node);
  for(int i=0; i<depth; i++)
    printf("  ");
 
  printf("Type %u Children %u ", (int)node->getType(node), count);
  if(node->getText(node)!=NULL)
    printf("%s\n", node->getText(node)->chars);
  else
    printf("empty\n");
  for(int i=0; i<count; i++)
    dumpTree((pANTLR3_BASE_TREE)node->getChild(node,i), depth+1);
}

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

//////////////////// Moving into an IR collection ///////////////////////////////

/* Data-storage for both declarations and parameters. 
   Both contain a type, identifier and specifiers. The only difference is that parameters 
   cannot contain an initialiser.
   TODO: Actually they should not be that similar - the parser is too loose and letting 
         through parameters with storage class specifiers when it should not.
*/
class Decl
{
public:
  char *identifier;
  bool typeStatic, typeExtern, typeTypedef, typeAuto, typeRegister;
  int  primType;    // -1 for things that are not
  int stars;
  int array;
  Decl() :
    primType(-1), stars(0), array(0)
  {
  }

  void clone(Decl *src) 
  {
    identifier   = src->identifier;   // Hairy - do not want this
    typeStatic   = src->typeStatic;
    typeExtern   = src->typeExtern;
    typeTypedef  = src->typeTypedef;
    typeAuto     = src->typeAuto;
    typeRegister = src->typeRegister;
    primType     = src->primType;
    stars        = src->stars;
    array        = src->array;
  }

  void parseSpecifiers(list<pANTLR3_BASE_TREE>::iterator start,list<pANTLR3_BASE_TREE>::iterator end)
  {
    while(start!=end)
    {
      pANTLR3_BASE_TREE tok = *start;
      switch(tok->getType(tok))
      {
        case FLOAT:
        case CHAR:
        case INT:
        case LONG:
          primType = tok->getType(tok);
          break;
        case AUTO:
          typeAuto = true;
          break;
        case TYPEDEF:
          typeTypedef = true;
          break;
        case EXTERN:
          typeExtern = true;
          break;
        case STATIC:
          typeStatic = true;
          break;
        case DECL:  break;  // Skip sub-trees
        default:
          if( tok->getText(tok) != NULL )
            printf("decl-->%s %d\n", (char*)tok->getText(tok)->chars, tok->getType(tok));
          else
            printf("decl-->empty %d\n", tok->getType(tok));
          break;
      }
      ++start;
    }
  }

  void parseInits(pANTLR3_BASE_TREE subTree)
  {
    int count = subTree->getChildCount(subTree);
    stars = 0;
    // There is no tree structure for stars in declarators so if a type is a pointer
    // there will be a stream of STAR tokens prefixing the identifier.
    for(int i=0; i<count; i++)
      if(getChildType(subTree,i)!=STAR)
      {
        stars = i;
        break;
      }

    if( stars >= count ) {
      printf("Malformed declaration - pointers but no id\n");
      return;
    }

    if( getChildType(subTree,stars) != IDENT ) {
      printf("Malformed declaration - missing IDENT\n");
      return;
    }
    pANTLR3_BASE_TREE id = (pANTLR3_BASE_TREE)subTree->getChild(subTree,stars);
    identifier = (char*)id->getText(id)->chars;
    bool firstUnexpected = true;
    for(int i=stars+1; i<count; i++)
    {
      pANTLR3_BASE_TREE tok = (pANTLR3_BASE_TREE)subTree->getChild(subTree,i);
      switch(tok->getType(tok))
      {
        case OPENSQ:
          if(i+2 >= count)
            printf("ERROR: truncated array expression\n");
          else
          {
            tok = (pANTLR3_BASE_TREE)subTree->getChild(subTree,++i);
            if(tok->getType(tok)!=NUM)
              printf("ERROR: array bound is unevaluated\n");
            else
              array = atoi((char*)tok->getText(tok)->chars);
            // Skip NUM CLOSESQ
            if(++i==count)
              printf("ERROR: truncated array expression\n");
          }
          break;
        case EQUALS:
          i = count;      // Skip initialiser expressions
          break;
        default:
          if(firstUnexpected) {
            printf("Unexpected child %d in subtree:\n", tok->getType(tok));
            dumpTree(subTree,0);
            firstUnexpected = false;
          }
          break;
      }
    }
  }

  /* The subtree for a DECL can contain multiple declarations in a comma-separated
     list. The initial children are specifiers, these will be cloned into every
     Decl produced. The results are appended to the list<Decl*> passed in.
  */
  static void parse(pANTLR3_BASE_TREE node, list<Decl*> &results)
  {
    list<pANTLR3_BASE_TREE> children = extractChildren(node,0,-1);
    Decl *first = new Decl;
    first->parseSpecifiers( children.begin(), children.end() );

    tmplForeach(list,pANTLR3_BASE_TREE,child,children)
      if( child->getType(child)==DECL )
      {
        Decl *d = new Decl;
        d->clone(first);
        d->parseInits(child);
        results.push_back(d);
      }
    tmplEnd
    //pANTLR3_BASE_TREE id = (pANTLR3_BASE_TREE)node->getChild(node,0);
    //identifier = (char*)id->getText(id)->chars;
    //parseTokenLs( rest.begin(), rest.end() );
  }

  string typeStr()
  {
    list<string> prefix;
    if(typeStatic)
      prefix.push_back("static");
    if(typeExtern)
      prefix.push_back("extern");
    if(typeTypedef)
      prefix.push_back("typedef");
    if(typeAuto)
      prefix.push_back("auto");
    if(typeRegister)
      prefix.push_back("register");
    switch(primType)
    {
      case CHAR:
        prefix.push_back("char");
        break;
      case FLOAT:
        prefix.push_back("float");
        break;
      case INT:
        prefix.push_back("int");
        break;
      case LONG:
        prefix.push_back("long");
        break;
    }
    stringstream res;
    res << joinStrings(prefix,' ');
    for(int i=0; i<stars; i++)
      res << '*';
    if(array>0)
      res << '[' << array << ']';
    return res.str();
  }
};

class Func
{
public:
  char *identifier;
  Decl retType;
  list<Decl*> params;
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
        Decl::parse(node,globals);
        //Decl *d = new Decl;
        //d->parseToken(node);
        //globals.push_back(d);
      }
      break;
    case FUNC:
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
            //Decl *p = new Decl;
            //p->parseToken(tok);
            //f->params.push_back(p);
          }
          else 
            break;
        }
        if(it!=rest.end())
        {
          //f->retType.parseTokenLs(it,rest.end());
        }
        functions.push_back(f);
      }
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

void TranslationU::dump()
{
  tmplForeach(list,Decl*,decl,globals)  
    printf("Declation: %s is %s\n", decl->identifier, decl->typeStr().c_str());
  tmplEnd
  tmplForeach(list,Func*,f,functions)  
    printf("Function: %s is ", f->identifier);
    printf("%s <- ", f->retType.typeStr().c_str());
    tmplForeach(list,Decl*,p,f->params)
      printf("%s  ", p->typeStr().c_str());
    tmplEnd
    printf("\n");
  tmplEnd
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

}
