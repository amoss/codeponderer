#include<list>
#include<sstream>
#include "models/c-repr.h"
#include "models/util.h"
#include<stdio.h>

using namespace std;

/*bool FtComp::operator() (FuncType const &a, FuncType const &b) const
{
static DtComp dc;

  if(a.nParams < b.nParams)
    return true;
  if(a.nParams > b.nParams)
    return false;
  if(dc(*a.retType,*b.retType))
    return true;
  if(dc(*b.retType,*a.retType))
    return false;
  for(int i=0; i<a.nParams; i++)
  {
    if(dc(*a.params[i], *b.params[i]))
      return true;
    if(dc(*b.params[i], *a.params[i]))
      return false;
  }
  return false;
}*/

TypeAtom::TypeAtom()
  : isUnsigned(false), isConst(false), stars(0), primitive(TypeAtom::Empty), array(0),
    tag(""), fidx(0)
{
}

/* Note: MUST initialise every field that is involved in the comparison otherwise it will
         produce an unstable ordering for std::set and all hell will break loose!
*/
/*DataType::DataType()
  : nFields(0), fields(NULL), namesp(NULL)
{
  // To avoid dependencies on c-init the intialisation is handled by the c-build module.
}

DataType::DataType(TypeAtom const &copy)
  : TypeAtom(copy)
{
}

bool compareFT(FuncType const &a, FuncType const &b)
{
}
*/

string TypeAtom::str() const
{
  list<string> parts;
  if(isConst)
    parts.push_back("const");
  if(isUnsigned)
    parts.push_back("unsigned");
  stringstream res;
  switch(primitive)
  {
    case TypeAtom::Empty:
      parts.push_back("EMPTY");
      break;
    case TypeAtom::Ellipsis:
      parts.push_back("...");
      break;
    case TypeAtom::Void:
      parts.push_back("void");
      break;
    case TypeAtom::Int:
      parts.push_back("int");
      break;
    case TypeAtom::Long:
      parts.push_back("long");
      break;
    case TypeAtom::Char:
      parts.push_back("char");
      break;
    case TypeAtom::Float:
      parts.push_back("float");
      break;
    case TypeAtom::Double:
      parts.push_back("double");
      break;
    case TypeAtom::Short:
      parts.push_back("short");
      break;
    case TypeAtom::Struct:
      parts.push_back("struct");
      parts.push_back(tag);
      break;
    case TypeAtom::Union:
      parts.push_back("union");
      parts.push_back(tag);
      break;
    case TypeAtom::Enum:
      parts.push_back("enum");
      break;
    case TypeAtom::Function:
      res << "ftype" << fidx;
      break;
    default:
      printf("Unknown prim %d\n",primitive);
      break;
  }
  res << joinStrings(parts,' ');
  for(int i=0; i<stars; i++)
    res << "*" ;
  for(int i=0; i<array; i++)
    res << "[]" ;
  return res.str();
}

bool TypeAtom::operator<(TypeAtom const &rhs) const
{
  if(primitive!=rhs.primitive)
    return primitive < rhs.primitive;
  if(stars!=rhs.stars)
    return stars < rhs.stars;
  if(array!=rhs.array)
    return array < rhs.array;
  if(isUnsigned != rhs.isUnsigned)
    return isUnsigned < rhs.isUnsigned;
  if(isConst != rhs.isConst)
    return isConst < rhs.isConst;
  if(tag != rhs.tag)
    return tag < rhs.tag;
  if(fidx != rhs.fidx)
    return fidx < rhs.fidx;
  return false;
}


string FuncType::str() const
{
stringstream res;
  res << retType.str();
  res << " <- ";
list<string> pstrs;
  if(nParams==0)
    res << "void";
  else 
  {
    for(int i=0; i<nParams; i++)
      pstrs.push_back(params[i].name + ":" + params[i].type.str());
    res << joinStrings(pstrs,',');
  }
  return res.str();
}
/*
Function::Function(FuncType &outside, SymbolTable *where)
{
  type = where->getCanon(outside);
  scope = new SymbolTable(where);
}


const DataType *SymbolTable::lookupSymbol(string name) const
{
map<string,const DataType*>::const_iterator it = symbols.find(name);
  if(it!=symbols.end())
    return it->second;
  if(parent!=NULL)
    return parent->lookupSymbol(name);
  return NULL;
}

const DataType *SymbolTable::lookupTag(string name) const
{
map<string,const DataType*>::const_iterator it = tags.find(name);
  if(it!=tags.end())
    return it->second;
  if(parent!=NULL)
    return parent->lookupTag(name);
  return NULL;
}
*/

set<TypeAtom> uniqueValues( map<string,TypeAtom> const &src)
{
map<string,TypeAtom>::const_iterator it;
set<TypeAtom> result;
  for(it=src.begin(); it!=src.end(); ++it)
    result.insert(it->second);
  return result;
}

void transposeValues(const char *prefix, map<string,TypeAtom> const &src, 
                     bool multiline=true)
{
set<TypeAtom> u = uniqueValues(src);
  tmplForeachConst(set, TypeAtom, t, u)
    printf("%s %s:", prefix, t.str().c_str());
    map<string,TypeAtom>::const_iterator it;
    for(it=src.begin(); it!=src.end(); ++it)
      if(it->second == t)
        printf(" %s", it->first.c_str());
    if(multiline)
      printf("\n");
    else
      printf(" ");
  tmplEnd
}

void SymbolTable::dump(bool justRecord)
{
  transposeValues("Decl", symbols, !justRecord);
  if(justRecord)
    return;
  transposeValues("Type", typedefs);
map<string,TypeAtom>::iterator it;
map<string,SymbolTable* >::iterator recIt;    
  for(recIt=tags.begin(); recIt!=tags.end(); ++recIt)
  {
    printf("Tag: %s -> {", recIt->first.c_str());
    recIt->second->dump(true);
    printf("}\n");
  }
unsigned int idx=0;
  tmplForeach(vector, FuncType, f, protos)
    printf("ftype%u : %s\n", idx++, f.str().c_str());
  tmplEnd
map<string,Function>::iterator fIt;
  for(fIt=functions.begin(); fIt!=functions.end(); ++fIt)
    printf("Func %s: ftype%u\n", fIt->first.c_str(), fIt->second.typeIdx);
}

bool SymbolTable::validTypedef(string name)
{
  if( typedefs.find(name)!=typedefs.end() )
    return true;
  if( parent!=NULL)
    return parent->validTypedef(name);
  return false;
}

TypeAtom SymbolTable::getTypedef(string name) 
{
  if( typedefs.find(name)!=typedefs.end() )
    return typedefs[name];
  if(parent!=NULL)
    return parent->getTypedef(name);
  return TypeAtom();
}

void SymbolTable::saveRecord(string name, SymbolTable *recTable)
{
  tags[name] = recTable;    // Should overwrite forward refs
}

void SymbolTable::saveType(string name, TypeAtom &t)
{
  typedefs[name] = t;
}

void SymbolTable::saveDecl(string name, TypeAtom &t)
{
  symbols[name] = t;
}

unsigned int SymbolTable::savePrototype(FuncType &f)
{
unsigned int idx = protos.size();
  protos.push_back(f);
  return idx;
}

void SymbolTable::saveFunction(string name, Function &f)
{
  functions.insert( pair<string,Function>(name,f) );
}

string SymbolTable::anonName()
{
stringstream res;
  res << "__anon" << anonCount++;
  return res.str();
}

TranslationU::TranslationU()
{
  table = new SymbolTable;
}

void TranslationU::dump()
{
/*
  tmplForeach(list,Decl*,decl,globals)  
    printf("Declation: %s is %s\n", decl->identifier, decl->type.str().c_str());
  tmplEnd
  tmplForeach(list,FuncDef*,f,functions)  
    printf("Function: %s is ", f->identifier);
    printf("%s <- ", f->retType.str().c_str());
    tmplForeach(list,Decl*,p,f->args)
      printf("%s ", p->type.str().c_str());
      printf("%s  ", p->identifier);
    tmplEnd
    printf("\n");
  tmplEnd
*/
}

void TranslationU::buildSymbolTable()
{
/*
  table = new SymbolTable;
  // Build canonical types
  // Resolve typenames in Types
  tmplForeach(list, Decl*, d, globals)
    string s = d->identifier;
    if( d->type.isTypedef )
      table->typedefs[s] = &d->type;
    else
    {
      if( d->type.primType == TYPEDEF )
      {
        printf("Resolve %s\n", d->type.typedefName);
        map<string,Type*>::iterator it = table->typedefs.find(d->type.typedefName);
        if( it==table->typedefs.end() )
          printf("Can't Resolve\n");
        else {
          printf("Resolved:", (it->second)->str().c_str());
        }
      }
      table->symbols[s] = &d->type;
    }
  tmplEnd
*/
}

