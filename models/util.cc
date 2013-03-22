#include "models/util.h"
//////////////////// Heading into some sort of misc.cc pile //////////////////////

// Implements the python str.join function on lists of strings. Awkward to inline without
// a helper function because the first iteration doesn't start with a separator so it needs
// to be lifted from the loop.
std::string joinStrings(std::list<std::string> &strs, char separator)
{
std::stringstream res;
std::list<std::string>::iterator it = strs.begin();
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
 
  printf("Type %u Children %u ", (int)node->getType(node), count);
  if(node->getText(node)!=NULL)
    printf("%s\n", node->getText(node)->chars);
  else
    printf("empty\n");
  for(int i=0; i<count; i++)
    dumpTree((pANTLR3_BASE_TREE)node->getChild(node,i), depth+1);
}



