#include "models/c-build.h"
using namespace std;

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

int main(int argc, char **argv)
{

TranslationU model = parseUnit(NULL, argv[1]);
  model.table->dump();
  
}
