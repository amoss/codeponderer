#include <map>
#include <string>
// In a context where typedef int Units;
// Should there be a distinction between:
//  int x;
//  Units y;
// ?
// : No - after all there is none in the program.

class FuncType;
class DataType
{
public:
  enum { Int, Long, Char, Float, Double, Short, Struct, Union, Enum, Func} primitive;
  bool isSigned;
  int  stars;
  int  array;
  FuncType *fptr;
};

bool compareDT(DataType const &a, DataType const &b);
bool compareFT(FuncType const &a, FuncType const &b);

class FuncType
{
public:
  int numParams;
  DataType *params;

};

// Merged ST for all headers
// TU level ST for globals
// One ST per block

class SymbolTable
{
public:
  SymbolTable *parent;
  std::map< std::string,DataType* > symbols;
  std::map< std::string,DataType* > typedefs;
  std::map< std::string,FuncType* > functions;
  // Canonical set of Types used in the ST (ie will also be owner of alised objects)
};

class TranslationU
{
public:
  SymbolTable *table;

  TranslationU();
  void dump();
  void buildSymbolTable();
};
