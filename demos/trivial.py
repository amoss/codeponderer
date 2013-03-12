import sys, antlr3
sys.path.append('generated')
import cInPyLexer as L
import cInPyParser as P
from cInPyTokens import symbols

def dumpTree(t, indent=0) :
  if t is None :
    print '  '*indent+'None'
    return
  print '  '*indent+str(t)
  for c in t.children :
    dumpTree(c,indent+1)

def buildModel(tree) :
  if isinstance(tree, antlr3.tree.CommonTree) :
    nodeType = tree.getType()
    # If there are multiple top-level symbols then ANTLR inserts a nil node as a container,
    # silently process.
    if nodeType==0 :
      for c in tree.children :
        buildModel(c)
      return
    if nodeType not in symbols :
      print nodeType, dir(tree)
      return
    if symbols[nodeType] == 'DECL' :
      print "Decl", tree.children
    elif symbols[nodeType] == 'FUNC' :
      print "Func", tree.children
    else :
      print dir(tree)
      print tree.getType()
      for c in tree.children :
        buildModel(c)
  else :
    print type(tree)

s = '''float x;
static int z;
char text[64] = "hello";
int bob(char x, char *harry) {
  stuff  { inside }
}'''

lex     = L.cInPyLexer(antlr3.StringStream(s))
parser  = P.cInPyParser(antlr3.CommonTokenStream(lex))
result  = parser.translationUnit()

buildModel(result.tree)
dumpTree(result.tree)

