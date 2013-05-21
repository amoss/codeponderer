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
   Split:       the fields within AtomType are separate from the main class so that they
                can be shared with PartialDataType in the build module.

   primitive==Function           <-> fptr!=NULL
   primitive in {Union,Struct}   <-> rptr!=NULL

   A formal model of the C type-system can be found in:
     C formalised in HOL. Michael Norrish. UCAM-CL-TR-453 ISSN 1476-2986 
   
*/



// The atomic part of a type, extensible parts are stored in the SymbolTable.
class TypeAtom
{
public:
  enum { Empty, Ellipsis, Int, Long, Char, Float, Double, Short, Struct, Union, Enum, 
         Void, Function } primitive;
  bool isUnsigned, isConst;
  int  stars;    // Levels of indirection
  int  array;    // Number of dimensions
  std::string tag;

  TypeAtom();
  //bool operator <(TypeAtom const &rhs) const;    Might not need....
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

/*
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
*/


/* All DataType objects referenced in params must be canonical instances owned by a SymbolTable
   object. It is assumed that FuncTypes can be shallow copied without problems.
*/
/*class FuncType
{
public:
  const DataType *retType;
  int nParams;
  const DataType **params;
  std::string    *paramNames;

  FuncType()
    : nParams(0), retType(NULL), params(NULL), paramNames(NULL)
  {
  }

  std::string str() const;

};
*/

/* These are unique (defs of functions) but we make the same shallow-copying assumptions
   because the scope pointer is private and the type is owned by the parent scope.
*/

class SymbolTable;
class Function
{
public:
  TypeAtom *ret;
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
class Decl
{
public:
  std::string name;
  TypeAtom type;
  Decl(std::string s, TypeAtom t)
    : name(s), type(t)
  {
  }
};
class SymbolTable
{
public:
  SymbolTable *parent;
  std::map< std::string,TypeAtom > symbols;
  std::map< std::string,TypeAtom > typedefs;
  std::map< std::string,std::list<Decl> > tags;       // Distinct names from typedefs
  //std::map< std::string,Function* > functions;  // Function definitions in this scope
  //std::map< std::string,FuncType* > funcRefs;   // Function types (ie pointers in this scope)
  /* As the above maps are defined over pointers we need a canonical address for a given type
     (no multiple copies even when types are aliased by multiple identifiers). The set container
     is guaranteed to be stable (references/iterators) wrt to insertion so we can use it as a
     canonical map (i.e. it is the inverse of an array value->unique address) */

  bool validTypedef(std::string);
  TypeAtom getTypedef(std::string);
  void saveRecord(std::string, std::list<Decl> &);
  void dump();
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
  SymbolTable *table;

  TranslationU();
  void dump();
  void buildSymbolTable();
};
