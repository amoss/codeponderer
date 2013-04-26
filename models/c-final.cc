#include "models/c-final.h"
#include<stdio.h>

using namespace std;

bool compareFT(FuncType const &a, FuncType const &b)
{
}

void DataType::dump()
{
  if(isUnsigned)
    printf("unsigned ");
  switch(primitive)
  {
    case DataType::Int:
      printf("int ");
      break;
    case DataType::Long:
      printf("long ");
      break;
    case DataType::Char:
      printf("char ");
      break;
    case DataType::Float:
      printf("float ");
      break;
    case DataType::Double:
      printf("double ");
      break;
    case DataType::Short:
      printf("short ");
      break;
    case DataType::Struct:
      printf("struct ");
      break;
    case DataType::Union:
      printf("union ");
      break;
    case DataType::Enum:
      printf("enum ");
      break;
    case DataType::Func:
      printf("func ");
      break;
  }
  for(int i=0; i<stars; i++)
    printf("*");
  printf("\n");
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
  {
    printf("Decl: %s -> %lx\n", it->first.c_str(), it->second);
    it->second->dump();
  }
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

