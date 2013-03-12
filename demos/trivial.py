import sys, antlr3
sys.path.append('generated')
import cInPyLexer as L
import cInPyParser as P

def dumpTree(t, indent=0) :
  if t is None :
    print '  '*indent+'None'
    return
  print '  '*indent+str(t)
  for c in t.children :
    dumpTree(c,indent+1)


s = '''float x;
static int z;
char text[64];
int bob(char x, char *harry) {
  stuff  { inside!! }
}'''

lex     = L.cInPyLexer(antlr3.StringStream(s))
parser  = P.cInPyParser(antlr3.CommonTokenStream(lex))
result  = parser.translationUnit()
dumpTree(result.tree)

