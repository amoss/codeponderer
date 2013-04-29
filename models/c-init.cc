#include "models/c-init.h"
using namespace std;

template<pANTLR3_BASE_TREE>
void partitionList(list<pANTLR3_BASE_TREE> src, list<pANTLR3_BASE_TREE> &yes, list<pANTLR3_BASE_TREE> &no, bool (*predicate)(pANTLR3_BASE_TREE) );

template<pANTLR3_BASE_TREE>
void takeWhile(list<pANTLR3_BASE_TREE>::iterator &it, list<pANTLR3_BASE_TREE>::iterator end, list<pANTLR3_BASE_TREE> &target, bool (*predicate)(pANTLR3_BASE_TREE));


/*REWRITE
FuncDef::FuncDef() :
  identifier(NULL)
{
}

extern pANTLR3_UINT8   cInCParserTokenNames[];
void FuncDef::parse(pANTLR3_BASE_TREE node)
{
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
}
*/


