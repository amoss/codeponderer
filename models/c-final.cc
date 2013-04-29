#include<list>
#include<sstream>
#include "models/c-final.h"
#include "models/util.h"
#include<stdio.h>

using namespace std;

bool DtComp::operator() (DataType const &a, DataType const &b) const
{
static FtComp fc;
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
  if(a.primitive!=DataType::Function)
    return false;
  return fc(*a.fptr, *b.fptr);
}

bool FtComp::operator() (FuncType const &a, FuncType const &b) const
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
}

/* Note: MUST initialise every field that is involved in the comparison otherwise it will
         produce an unstable ordering for std::set and all hell will break loose!
*/
DataType::DataType()
  : nFields(0), isUnsigned(false), fields(NULL), stars(0), primitive(DataType::Empty), array(0)
{
  // To avoid dependencies on c-init the intialisation is handled by the c-build module.
}

bool compareFT(FuncType const &a, FuncType const &b)
{
}

string DataType::str() const
{
  list<string> parts;
  if(isUnsigned)
    parts.push_back("unsigned");
  switch(primitive)
  {
    case DataType::Empty:
      parts.push_back("EMPTY");
      break;
    case DataType::Ellipsis:
      parts.push_back("...");
      break;
    case DataType::Void:
      parts.push_back("void");
      break;
    case DataType::Int:
      parts.push_back("int");
      break;
    case DataType::Long:
      parts.push_back("long");
      break;
    case DataType::Char:
      parts.push_back("char");
      break;
    case DataType::Float:
      parts.push_back("float");
      break;
    case DataType::Double:
      parts.push_back("double");
      break;
    case DataType::Short:
      parts.push_back("short");
      break;
    case DataType::Struct:
      parts.push_back("struct");
      break;
    case DataType::Union:
      parts.push_back("union");
      break;
    case DataType::Enum:
      parts.push_back("enum");
      break;
    case DataType::Function:
      parts.push_back("func");
      break;
    default:
      printf("Unknown prim %d\n",primitive);
      break;
  }
  stringstream res;
  res << joinStrings(parts,' ');
  for(int i=0; i<stars; i++)
    res << "*" ;
  if(nFields > 0)
  {
    for(int i=0; i<nFields; i++ )
        res << fields[i]->str() << ";";
  }
  return res.str();
}

const DataType *SymbolTable::getCanon(DataType const &src)
{
  canon.insert(src);
  set<DataType,DtComp>::iterator it = canon.find(src);
  return &(*it);
}

FuncType *SymbolTable::getCanon(FuncType const &src)
{
  canonF.insert(src);
  set<FuncType,FtComp>::iterator it = canonF.find(src);
  return (FuncType*)&(*it);
}

void SymbolTable::dump()
{
map<string,const DataType*>::iterator it;
  for(it=symbols.begin(); it!=symbols.end(); ++it)
    printf("Decl: %s -> %lx = %s\n", it->first.c_str(), it->second, it->second->str().c_str());
  for(it=typedefs.begin(); it!=typedefs.end(); ++it)
    printf("Type: %s -> %lx = %s\n", it->first.c_str(), it->second, it->second->str().c_str());
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

