#include<list>
#include<sstream>
#include "models/c-final.h"
#include "models/util.h"
#include<stdio.h>

using namespace std;

bool compareFT(FuncType const &a, FuncType const &b)
{
}

string DataType::str()
{
  list<string> parts;
  if(isUnsigned)
    parts.push_back("unsigned");
  switch(primitive)
  {
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
    case DataType::Func:
      parts.push_back("func");
      break;
  }
  stringstream res;
  res << joinStrings(parts,' ');
  for(int i=0; i<stars; i++)
    res << "*" ;
  return res.str();
}

DataType *SymbolTable::getCanon(DataType const &src)
{
  canon.insert(src);
  set<DataType,DtComp>::iterator it = canon.find(src);
  return (DataType*)&(*it);
}

void SymbolTable::dump()
{
map<string,DataType*>::iterator it;
  for(it=symbols.begin(); it!=symbols.end(); ++it)
    printf("Decl: %s -> %lx = %s\n", it->first.c_str(), it->second, it->second->str().c_str());
  for(it=typedefs.begin(); it!=typedefs.end(); ++it)
    printf("Type: %s -> %lx = %s\n", it->first.c_str(), it->second, it->second->str().c_str());
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

