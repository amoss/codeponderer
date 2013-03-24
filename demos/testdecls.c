// Primitive types with and without initialisers
char cx, cy = 'z', cz[4] = "abc";
int  ix = -33, iy, iz[2] = { 0x1234, 55 };
long lx = -33, ly, lz[2] = { 0x1234, 55 };
float fx, fy = 33.125, fz[2] = { 0.5, 0.7 };
double dx, dy = 33.125, dz[2] = { 0.5, 0.7 };

// Mixture of specifiers
const int n=7;
static const char c='?';
extern float ff;
unsigned int zero=0;

// Some typedefs
typedef unsigned char /* check comments inline */ Byte; 
typedef int /*different comment*/ Map(int);
Byte b;
Should Not Parse;

// Function defs
void kim() {}
void kimson() { CRAP }
int bobette(char y) { }
int bob(char x, char *harry) {      DOESNT MATCH
  ignored stuff  { inside matched scopes }
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
int proto5(int *, int **);
void (*proc1)();
typedef void (*proc)();
int reg(void (*callback)());

// Broken stuff
//*ptr;
(int *ptr2b);            // Is this valid?
//(*ptr2);            // Is this valid?
// NO nasty(*,*,*);       // Is this valid?
// NO ugly(,,,);          // ?
int (*fp1) (int);     // NOT EVEN CLOSE
int * (* (*fp2) (int) );     // NOT EVEN CLOSE
int * (* (*fp3) (int) ) [10];     // NOT EVEN CLOSE

typedef struct 
{
    int x,y;
} point;
