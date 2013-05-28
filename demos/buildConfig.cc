#include <map>
#include "models/c-build.h"
#include "misc/path.h"
using namespace std;

/* Build Configurations for real code are non-trivial.

   While the trivial demo shows how simple it is to handle a single source unit 
   (including headers if the pre-processor output is used) this demo shows how
   to simulate a more realistic build environment.
*/


// Global state for callback
SymbolTable *systemST, *projectST;
map<string,TranslationU> project;
string origName;

SymbolTable *classify(TranslationU *context, Path const &where)
{
// Relative path when the preprocessor was invoked
static Path prjRoot = Path("../gawk-4.0.2");
  printf("ctx %s where %s orig %s\n", context->path.c_str(), where.repr().c_str(), origName.c_str());
  if( where.inside(prjRoot) )
  {
    if( where.repr() == origName )
      return context->table;
    else
      return projectST;
  }
  return systemST;
}

pair<string,char *> ppMap[] =
{
  pair<string,char *>("../gawk-4.0.2/array.c", "testcases/pp-gawk/array.i")
};

int main(int argc, char **argv)
{
  systemST  = new SymbolTable();
  projectST = new SymbolTable(systemST);

  for(int i=0; i<sizeof(ppMap)/sizeof(pair<string,string>); i++)
  {
    try 
    {
      origName = ppMap[i].first;
      project.insert( pair<string,TranslationU>(ppMap[i].first, parseUnit(projectST, ppMap[i].second, classify)) );
    }
    catch(BrokenTree bt) 
    {
      printf("Exception %s on %s:%u\n", bt.explain, ppMap[i].first.c_str(), bt.blame->getLine(bt.blame));
      dumpTree(bt.blame,1);
    }
  }
  printf("------------> System:\n");
  systemST->dump();
  printf("\n-----------> Project:\n");
  projectST->dump();
  for(map<string,TranslationU>::iterator tu=project.begin(); tu!=project.end(); ++tu)
  {
    printf("\n----------->Translation Unit: %s\n", tu->first.c_str());
    tu->second.table->dump();
  }
}
