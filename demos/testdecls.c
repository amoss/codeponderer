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

// Function defs
int bob(char x, char *harry) {
  ignored stuff  { inside matched scopes }
}

// Pointers
int *p1, **p2, ***p3;
const char *cc;
char * const CC;

// Function pointers
int * (* (*fp1) (int) ) [10];
typedef void (*proc)();

typedef struct 
{
    int x,y;
} point;
