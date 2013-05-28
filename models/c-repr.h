#include <set>
#include <map>
#include <string>
#include <vector>

#include "misc/path.h"

/*   A formal model of the C type-system can be found in:
     C formalised in HOL. Michael Norrish. UCAM-CL-TR-453 ISSN 1476-2986 
*/

// The atomic part of a type, extensible parts are stored in the SymbolTable.
class TypeAtom
{
public:
  enum { Empty, Ellipsis, Int, Long, Char, Float, Double, Short, Struct, Union, Enum, 
         Void, Function } primitive;
  bool isUnsigned, isConst;
  int  stars;         // Levels of indirection
  int  array;         // Number of dimensions
  std::string tag;    // Lookup for union/struct

  unsigned int fidx;          // Index of function-type
  TypeAtom();
  bool operator <(TypeAtom const &rhs) const;
  bool operator==(TypeAtom const &rhs) const
  {
    return primitive==rhs.primitive && isUnsigned==rhs.isUnsigned && isConst==rhs.isConst
        && stars==rhs.stars && array==rhs.array && tag==rhs.tag && fidx==rhs.fidx;
  }
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

class Decl
{
public:
  std::string name;
  TypeAtom type;
  Decl(std::string s="", TypeAtom t=TypeAtom())
    : name(s), type(t)
  {
  }
};

class FuncType
{
public:
  TypeAtom retType;
  int nParams;
  Decl *params;

  FuncType()
    : nParams(0), params(NULL)
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
  unsigned int typeIdx;
  // Record for parameters
  SymbolTable *scope;
  Function(SymbolTable *where)
    : scope(where)
  {
  }
  
};


// Merged ST for all headers
// TU level ST for globals
// One ST per block

/*Type :: Primitive Stars Arrays
      | (RecordPrim tag) vs (RecordPrim [Decl])
      ?? on anon -> generate unique tag ?!?
      | Unresolved (forward-refs)
Decl :: Name Type
ST   :: Typedefs -> Type ; Tags -> Type ; [Decl]

eq :: (Primitive Stars Arrays) ==
    | RecordPrim same tree shape in Decl list.
*/
class SymbolTable
{
public:
  SymbolTable *parent;
  std::map< std::string,TypeAtom > symbols;
  std::map< std::string,TypeAtom > typedefs;
  std::map< std::string,SymbolTable* > tags;       // Distinct names from typedefs
  std::vector< FuncType > protos;
  std::map< std::string,Function > functions;  // Function definitions in this scope
  //std::map< std::string,FuncType* > funcRefs;   // Function types (ie pointers in this scope)
  /* As the above maps are defined over pointers we need a canonical address for a given type
     (no multiple copies even when types are aliased by multiple identifiers). The set container
     is guaranteed to be stable (references/iterators) wrt to insertion so we can use it as a
     canonical map (i.e. it is the inverse of an array value->unique address) */

  bool validTypedef(std::string);
  TypeAtom getTypedef(std::string);
  void saveRecord(std::string, SymbolTable *);
  void saveType(std::string, TypeAtom &);
  void saveDecl(std::string, TypeAtom &);
  unsigned int savePrototype(FuncType &);
  void saveFunction(std::string, Function &);
  void dump(bool justRecord=false);
  /*
  const DataType *getCanon(DataType const &);     // Inserts if not present
  FuncType *getCanon(FuncType const &);           // Inserts if not present
  const DataType *lookupTag(std::string) const;
  const DataType *lookupTypedef(std::string) const;
  const DataType *lookupSymbol(std::string) const;
*/
  int anonCount;
  SymbolTable(SymbolTable *p=NULL) :
    parent(p), anonCount(1)
  {
  }
  std::string anonName();
};

class TranslationU
{
public:
typedef SymbolTable *(*PathClassifier)(TranslationU *,Path const &);
  SymbolTable *table;
  std::string path;

  PathClassifier headers;
  TranslationU(SymbolTable *, std::string, PathClassifier=NULL);
  void dump();
};
