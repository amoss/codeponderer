import sys, antlr3
sys.path.append('generated')
import cInPyLexer as L
import cInPyParser as P
#from cInPyTokens import symbols

def dumpTree(t, indent=0) :
  if t is None :
    print '  '*indent+'None'
    return
  print '  '*indent+str(t)
  for c in t.children :
    dumpTree(c,indent+1)

class Type(object) :
  pass

class Declaration(object) :
  pass

class FunctionDef(object) :
  pass 


def buildModel(tree) :
  if isinstance(tree, antlr3.tree.CommonTree) :
    nodeType = tree.getType()
    # If there are multiple top-level symbols then ANTLR inserts a nil node as a container,
    # silently process.
    if nodeType==0 :
      for c in tree.children :
        buildModel(c)
      return
    #if nodeType not in symbols :
    #  print nodeType, dir(tree)
    #  return
    if nodeType == L.DECL :
      print "Decl", tree.children
    elif nodeType == L.FUNC :
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
}
int * (* (*fp1) (int) ) [10];
int n;
int *p;
int* p,q;
char **argv;
int RollNum[30][4];
int (*p)[4]=RollNum;
int *q[5];
int **p1;  //  p1 is a pointer   to a pointer   to an int.
int *&p2;  //  p2 is a reference to a pointer   to an int.
int &*p3;  //  ERROR: Pointer    to a reference is illegal.
int &&p4;  //  ERROR: Reference  to a reference is illegal.
const int n=5;
int const m=10;
const int *p;
int const *q;
int * const r= &n; // n has been declared as an int
const int * const p=&n // n has been declared as const int
char ** p1;                    //        pointer to       pointer to       char
const char **p2;               //        pointer to       pointer to const char
char * const * p3;             //        pointer to const pointer to       char
const char * const * p4;       //        pointer to const pointer to const char
char ** const p5;              //  const pointer to       pointer to       char
const char ** const p6;        //  const pointer to       pointer to const char
char * const * const p7;       //  const pointer to const pointer to       char
const char * const * const p8; //  const pointer to const pointer to const char
typedef char * PCHAR;
PCHAR p,q;
typedef char * a;  // a is a pointer to a char
typedef a b();     // b is a function that returns
typedef b *c;      // c is a pointer to a function
typedef c d();     // d is a function returning
typedef d *e;      // e is a pointer to a function 
e var[10];         // var is an array of 10 pointers to 
typedef struct tagPOINT
{
    int x;
    int y;
}POINT;
POINT p; /* Valid C code */
int (*p)(char);
char ** (*p)(float, float);
void * (*a[5])(char * const, char * const);
int * (* (*fp1) (int) ) [10];
int *( *( *arr[5])())();
float ( * ( *b()) [] )();              // b is a function that returns a 
void * ( *c) ( char, int (*)());       // c is a pointer to a function that takes
void ** (*d) (int &, 
  char **(*)(char *, char **));        // d is a pointer to a function that takes
float ( * ( * e[10]) 
    (int &) ) [5];                    // e is an array of 10 pointers to 
}'''

lex     = L.cInPyLexer(antlr3.StringStream(s))
parser  = P.cInPyParser(antlr3.CommonTokenStream(lex))
result  = parser.translationUnit()

buildModel(result.tree)
dumpTree(result.tree)

