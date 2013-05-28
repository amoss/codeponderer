#include <list>
#include <exception>
#include "misc/util.h"
#include "models/c-repr.h"
#include "models/graph.h"

TranslationU parseUnit(SymbolTable *parent, char *filename, TranslationU::PathClassifier = NULL);

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

/*
  std::string str() const;
  bool operator <(PartialDataType const &rhs) const
  {
    if( TypeAtom::operator<(rhs) )
      return true;
    if( static_cast<TypeAtom const&>(rhs) < *this )
      return false;
    // Test both ways as only equality on base-fields should fall-through
    switch(primitive)
    {
      case DataType::Struct:
      case DataType::Union:
        return tag < rhs.tag;
      default:
        return false;
    }
  }
*/



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

