#include "models/c-final.h"
// Arbitrary ordering for Type objects that models equality for set-inclusion
bool compareDT(DataType const &a, DataType const &b)
{
  if(a.primitive < b.primitive)
    return true;
  if(b.primitive < a.primitive)
    return false;
  if(a.stars < b.stars)
    return true;
  if(b.stars < a.stars)
    return false;
  if(a.isSigned && !b.isSigned)
    return true;
  if(!a.isSigned && b.isSigned)
    return false;
  if(a.array < b.array)
    return true;
  if(b.array < a.array)
    return false;
  if(a.primitive!=DataType::Func)
    return false;
  return compareFT(*a.fptr, *b.fptr);
}

bool compareFT(FuncType const &a, FuncType const &b)
{
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

