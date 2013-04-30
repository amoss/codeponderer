// Primitive types with and without initialisers
char cx, cy = 'z', cz[4] = "abc";
int  ix = -33, iy, iz[2] = { 0x1234, 55 };
long lx = -33, ly, lz[2] = { 0x1234, 55 };
float fx, fy = 33.125, fz[2] = { 0.5, 0.7 };
double dx, dy = 33.125, dz[2] = { 0.5, 0.7 };

// String constants
static char metas[] = ".*+(){}[]|?^$\\";
static char metas2[] = ".*+(){}[]|?^$\\\"";

// Mixture of specifiers
const int n=7;
static const char c='?';
extern float ff;
unsigned int zero=0;

// Some typedefs
typedef unsigned char /* check comments inline */ Byte; 
typedef int /*different comment*/ Map(int);
Byte b;
//Should Not Parse;
void blah(Byte);

// Function defs
void kim() {}
void kimson() { CRAP; }
int bobette(char y) { }
int bob(char x, char *harry) {      DOESNT MATCH;
  ignored stuff;  { inside matched scopes; }
} 

char **realcode()
{
int x;
  for(int i=0; i<7; i++)
  {
    char *xx;
    xx = x+i;
  }
  printf("Hello");
  while(true)
    break;
}

// Pointers
int *p1, **p2, ***p3;
const char *cc;
char * const CC; 

// Function pointers and protos
int proto1(char);
int proto2(char name);
int proto3(int blah, char name);
int proto4(int *a, int **b);
int *proto5(int *, int **);
void mixing(int, int **, char x);
void (*proc1)();
int (*fp1) (int);     
int **(*messy)(int *);
int (*nasty(int))();    // Returns a fptr -> int f(int)
int (*nasty2(int,int))(int*,int(*arg)(int));    // Returns a fptr -> int f(int,int)
typedef void (*proc)();
int reg(void (*callback)());
int reg(void (*callback)())
{
}

// Broken stuff
//*ptr;
//int * (* (*fp2) (int) );     // NOT EVEN CLOSE
//int * (* (*fp3) (int) ) [10];     // NOT EVEN CLOSE

// Structures
struct a { int crap; } x;
struct a y;
struct { char a,b,c; } triple;
struct triple { char a,b,c; };
struct a first,second;
struct wrapping { struct a first,second; };
union Weirdness { int x; char y; struct triple blah;};

typedef struct 
{
    int x,y;
} point;
