import sys, antlr3
sys.path.append('generated')
import cInPyLexer as L
import cInPyParser as P

s = '''float x;
int bob(char x, char *harry) {
  stuff
}'''

lex     = L.cInPyLexer(antlr3.StringStream(s))
parser  = P.cInPyParser(antlr3.CommonTokenStream(lex))
result  = parser.translationUnit()
print result

