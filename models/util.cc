#include "models/util.h"
using namespace std;

extern pANTLR3_UINT8   cInCParserTokenNames[];

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

/* Utility function: extract a slice of the children into a list */
TokList extractChildren(pANTLR3_BASE_TREE node, int lo, int hi)
{
TokList result;
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
 
  int t = (int)node->getType(node); 
  printf("Type %u %s Children %u ", t, cInCParserTokenNames[t], count);
  if(node->getText(node)!=NULL)
    printf("%s\n", node->getText(node)->chars);
  else
    printf("empty\n");
  for(int i=0; i<count; i++)
    dumpTree((pANTLR3_BASE_TREE)node->getChild(node,i), depth+1);
}


/* TODO:
   There are some interesting comments about getText memory leaks on the mailing
   list. Might want to wrap all of the getText() handling into a single function
   that returns a string and ensures the C runtime side is freed.
*/
void printTokList(TokList ls)
{
  tmplForeach(list, pANTLR3_BASE_TREE, tok, ls)
    printf("%d ", tok->getType(tok));
    pANTLR3_STRING txt = tok->getText(tok);
    if(txt!=NULL)
      printf("%s ", txt->chars);
  tmplEnd
  printf("\n");
}


