#include<list>
#include<string>
#include<sstream>
extern "C" {
#include "cInCLexer.h"
#include "cInCParser.h"
}


// C++11 is scary and both "differently" supported and "differently" able.
// Also upgrading g++ on the knackered old version of Ubuntu that I use can wait until another day.
#define foreach(T,V,C) for(T::iterator V = C.begin(); V!=C.end(); ++V)
// This is definitely a bit messy... but it works :)
#define tmplForeach(Tmpl,ElemT,V,C) { Tmpl<ElemT>::iterator V##It; ElemT V; for(V##It = C.begin(),V=*V##It; V##It!=C.end(); ++V##It,V=*V##It) {
#define tmplEnd } }

typedef std::list<pANTLR3_BASE_TREE> TokList;

std::string joinStrings(std::list<std::string> &strs, char separator);
TokList extractChildren(pANTLR3_BASE_TREE node, int lo, int hi);
void dumpTree(pANTLR3_BASE_TREE node, int depth);
void printTokList(TokList ls);

/* Basically does takeWhile / dropWhile in parallel for functional-style splits 
   without relying on C11 lambda type stuff (they look nasty) */
template<typename T>
void splitList(std::list<T> src, std::list<T> &yes, std::list<T> &no, bool (*predicate)(T) )
{
  typename std::list<T>::iterator elIt = src.begin();
  for(;elIt!=src.end(); ++elIt)
  {
    if( predicate(*elIt) )
      yes.push_back(*elIt);
    else 
      no.push_back(*elIt);
  }
}

/* Do a takeWhile into the first list using the predicate, copy the rest into
   the second list */
template<typename T>
void partitionList(std::list<T> src, std::list<T> &yes, std::list<T> &no, bool (*predicate)(T) )
{
  typename std::list<T>::iterator elIt = src.begin();
  for(;elIt!=src.end(); ++elIt)
  {
    if( predicate(*elIt) )
      yes.push_back(*elIt);
    else
      break;
  }
  for(;elIt!=src.end(); ++elIt)
      no.push_back(*elIt);
}
