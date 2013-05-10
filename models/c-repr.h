#include <set>
#include <map>
#include <string>

/* The representation of a type in the program. Note the lack of STL containers, these
   types are immutable - the only time that fields / functions etc are added is during
   construction (conversion from the parser structures). Sticking to pointer arrays 
   and integer counts leads to easier code to read/write than STL iterators. These objects
   are shallow-copiable as all pointers are canoncial instances owned by the SymbolTable.
   Empty is a null-object and Ellipsis is a special dummy in the SymbolTable to catch
   vargs in functions. 

   Homogeneity: every type in the C type-system can be represented by instances of this
                class, those types that represent radically different entities (i.e.
                functions are callable, records can be indexed) have a 
                split-representation with the more specific representation hidden behind
                a pointer. This allows DataTypes to be ordered, and thus allows a 
                canonical store for each symbol table that removes aliasing of types.

   primitive==Function           <-> fptr!=NULL
   primitive in {Union,Struct}   <-> rptr!=NULL
*/
class FuncType;
class RecType;
class SymbolTable;
class DataType
{
public:
  enum { Empty, Ellipsis, Int, Long, Char, Float, Double, Short, Struct, Union, Enum, 
         Void, Function } primitive;
  bool isUnsigned, isConst;
  int  stars;    // Levels of indirection
  int  array;    // Number of dimensions
  FuncType *fptr;
  RecType  *rptr;

  DataType();
  std::string str() const;
};

/* Stuff that we parse but throw away (class used internally in c-build).

   Some of this simply doesn't fit in the model for the type-system, and instead is
   used when building the collections of DataTypes. Some of this we just don't care
   about.
*/
class TypeAnnotation
{
public:
  bool isAuto, isExtern, isStatic, isVolatile, isInline, isTypedef;
  TypeAnnotation()
    : isAuto(false), isExtern(false), isStatic(false), isVolatile(false), isInline(false),
      isTypedef(false)
  {
  }
};

// Arbitrary ordering for Type objects that models equality for set-inclusion
class DtComp
{
public:
  bool operator() (DataType const &a, DataType const &b) const;
};

class FtComp
{
public:
  bool operator() (FuncType const &a, FuncType const &b) const;
};

class RtComp
{
public:
  bool operator() (RecType const &a, RecType const &b) const;
};


/* All DataType objects referenced in params must be canonical instances owned by a SymbolTable
   object. It is assumed that FuncTypes can be shallow copied without problems.
*/
class FuncType
{
public:
  const DataType *retType;    // Owned by the SymbolTable that owns the wrapper for this
  int nParams;
  const DataType **params;    // Owned by the SymbolTable that owns the wrapper for this
  std::string    *paramNames;

  FuncType()
    : nParams(0), retType(NULL), params(NULL), paramNames(NULL)
  {
  }

  std::string str() const;

};

/* These are unique (defs of functions) but we make the same shallow-copying assumptions
   because the scope pointer is private and the type is owned by the parent scope.
*/
class SymbolTable;
class Function
{
public:
  FuncType *type;
  SymbolTable *scope;
  Function(FuncType &outside, SymbolTable *where);
  
};

class RecType
{
public:
  RecType();
  int  nFields;
  const DataType **fields;
  SymbolTable *namesp;  // Record types have a private namespace, fields are canon within.
                        // The shallow copy guarantee is a bit wobbly but should still hold...
  std::string str() const;

};

// Merged ST for all headers
// TU level ST for globals
// One ST per block

class SymbolTable
{
public:
  SymbolTable *parent;
  std::map< std::string,const DataType* > symbols;
  std::map< std::string,const DataType* > typedefs;
  std::map< std::string,const RecType* > tags;   // Distinct names from typedefs
  std::map< std::string,Function* > functions;  // Function definitions in this scope
  std::map< std::string,FuncType* > funcRefs;   // Function types (ie pointers in this scope)
  /* As the above maps are defined over pointers we need a canonical address for a given type
     (no multiple copies even when types are aliased by multiple identifiers). The set container
     is guaranteed to be stable (references/iterators) wrt to insertion so we can use it as a
     canonical map (i.e. it is the inverse of an array value->unique address) */
  std::set< DataType, DtComp>       canon;
  std::set< FuncType, FtComp>       canonF;
  const DataType *getCanon(DataType const &);     // Inserts if not present
  FuncType *getCanon(FuncType const &);           // Inserts if not present
  const RecType *lookupTag(std::string) const;
  const DataType *lookupTypedef(std::string) const;
  const DataType *lookupSymbol(std::string) const;
  void dump();
  SymbolTable(SymbolTable *p=NULL) :
    parent(p)
  {
  }
};

class TranslationU
{
public:
  SymbolTable *table;

  TranslationU();
  void dump();
  void buildSymbolTable();
};
