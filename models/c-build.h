#include <list>
#include <exception>
#include "models/util.h"
#include "models/c-repr.h"

TranslationU parseUnit(char *filename);

class BrokenTree : public std::exception
{
public:
  pANTLR3_BASE_TREE blame;
  const char *explain;
  BrokenTree(pANTLR3_BASE_TREE node, const char *text)
    :  blame(node), explain(text)
  {
  }
  const char *what() const throw () { return explain; }
};

/* To allow for forward references (co-recursion in record definitions) this wrapper
   stores the tag-names of unresolved fields.
*/
class PartialDataType : public DataType
{
public:
  bool partial;

  // Case 1.
  //    struct fwd *blah;
  // Case 2.
  //    struct { struct fwd *blah; }
  // Case 3.
  //    void f( struct fwd *blah; }


  // Partial = DataType (value rather than const* )  |  tagname (must be record-type)...

  // Decl :=
  //   Partial name 
  // can either inject into SymbolTable or stick in a list...

  std::string tag;
  std::string typedefName;

  std::string **structFields;     // Become overlay / wrapper for fields in DataType
  std::string **unionFields;
  // Empty value
  PartialDataType() 
    : DataType(), partial(false), structFields(NULL), unionFields(NULL)
  {
  }
  // Embed value
  PartialDataType(DataType const &copy)
    : DataType(copy), partial(false), structFields(NULL), unionFields(NULL)
  {
  }
  void finalise(SymbolTable *st, std::string name, TypeAnnotation ann);
};

class Decl
{
public:
  std::string name;
  PartialDataType type;
  Decl(std::string n, PartialDataType t) 
    : name(n), type(t)
  {
  }
};

class PartialState
{
public:
  std::list<PartialDataType> defs;
  std::list<PartialDataType> decls;
  bool findTag(std::string tag);
};

// TODO:
// How many places access the SymbolTable directly?
//    Merge these into a single point: PartialDataType::finalise?
// How to store the parts that cannot currently be resolved?
//    Class with partial records and decls?
// How to finalise (compute isomorphisms of) co-recursive types?
// Ideas:
//   Current problems are a form of pollution - what is the slice of the
//   code that touches a given type. Would be interesting to compute this
//   within the analyser...
// This would have been useful at the start...
//@inproceedings{telea2009extraction,
//  title={Extraction and visualization of call dependencies for large C/C++ code bases: A comparative study},
//  author={Telea, Alexandru and Hoogendorp, Hessel and Ersoy, Ozan and Reniers, Dennie},
//  booktitle={Visualizing Software for Understanding and Analysis, 2009. VISSOFT 2009. 5th IEEE International Workshop on},
//  pages={81--88},
//  year={2009},
//  organization={IEEE}
//}

