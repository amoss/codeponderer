#include <set>
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
  bool isUnsigned;
  int  stars;
  int  array;
  FuncType *fptr;
  std::string str();
};

// Arbitrary ordering for Type objects that models equality for set-inclusion
bool compareFT(FuncType const &a, FuncType const &b);
class DtComp
{
public:
  bool operator() (DataType const &a, DataType const &b) const
  {
    if(a.primitive < b.primitive)
      return true;
    if(b.primitive < a.primitive)
      return false;
    if(a.stars < b.stars)
      return true;
    if(b.stars < a.stars)
      return false;
    if(a.isUnsigned && !b.isUnsigned)
      return true;
    if(!a.isUnsigned && b.isUnsigned)
      return false;
    if(a.array < b.array)
      return true;
    if(b.array < a.array)
      return false;
    if(a.primitive!=DataType::Func)
      return false;
    return compareFT(*a.fptr, *b.fptr);
  }
};


class FuncType
{
public:
  int numParams;
  DataType *params;

};

class SymbolTable;
class Function
{
public:
  FuncType type;
  SymbolTable *scope;
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
  std::map< std::string,Function* > functions;  // Function definitions in this scope
  std::map< std::string,FuncType* > funcRefs;   // Function types (ie pointers in this scope)
  /* As the above maps are defined over pointers we need a canonical address for a given type
     (no multiple copies even when types are aliased by multiple identifiers). The set container
     is guaranteed to be stable (references/iterators) wrt to insertion so we can use it as a
     canonical map (i.e. it is the inverse of an array value->unique address) */
  std::set< DataType, DtComp>       canon;
  DataType *getCanon(DataType const &);
  void dump();
};

class TranslationU
{
public:
  SymbolTable *table;

  TranslationU();
  void dump();
  void buildSymbolTable();
};
