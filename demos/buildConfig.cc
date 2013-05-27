#include "models/c-build.h"
#include "misc/path.h"
/* Build Configurations for real code are non-trivial.

   While the trivial demo shows how simple it is to handle a single source unit 
   (including headers if the pre-processor output is used) this demo shows how
   to simulate a more realistic build environment.
*/


SymbolTable *systemST, *projectST;
SymbolTable *classify(TranslationU *context, Path const &where)
{
//  if(...)
    return systemST;
//  if(...)
    return projectST;
  return context->table;
}

int main(int argc, char **argv)
{
  systemST  = new SymbolTable();
  projectST = new SymbolTable(systemST);
}
