#include<list>
#include<sstream>
#include "models/c-repr.h"
#include "models/util.h"
#include<stdio.h>

using namespace std;

/*bool DtComp::operator() (DataType const &a, DataType const &b) const
{
static FtComp fc;
  // Test both ways as only equality on base-fields should fall-through
  if( static_cast<TypeAtom const&>(a) < static_cast<TypeAtom const&>(b) )
    return true;
  if( static_cast<TypeAtom const&>(b) < static_cast<TypeAtom const&>(a) )
    return false;

  if(a.primitive==DataType::Struct || a.primitive==DataType::Union)
  {
    if(a.nFields < b.nFields)
      return true;
    if(a.nFields > b.nFields)
      return false;
    for(int i=0; i<a.nFields; i++)
    {
      if( (*this)(*a.fields[i], *b.fields[i]) )  // DtComp is stateless so reuse object
        return true;
      if( (*this)(*b.fields[i], *a.fields[i]) )
        return false;
    }
    return false;
  }
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
}*/

TypeAtom::TypeAtom()
  : isUnsigned(false), isConst(false), stars(0), primitive(TypeAtom::Empty), array(0),
    tag("")
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
      //parts.push_back(fptr->retType->str());
      break;
    default:
      printf("Unknown prim %d\n",primitive);
      break;
  }
  stringstream res;
  res << joinStrings(parts,' ');
  for(int i=0; i<stars; i++)
    res << "*" ;
  return res.str();
}

/*bool TypeAtom::operator<(TypeAtom const &rhs) const
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
  return false;
}*/

/*string DataType::str() const
{
stringstream res;
  res << ((TypeAtom)*this).str();
  if(primitive==DataType::Function)
  {
    res << '(';
    res << fptr->str();
    res << ')';
  }
  if(nFields > 0)
  {
    res << '{';
    for(int i=0; i<nFields; i++ )
        res << fields[i]->str() << ";";
    res << '}';
  }
  for(int i=0; i<array; i++)
    res << "[]";
  return res.str();
}

string FuncType::str() const
{
stringstream res;
  res << retType->str();
  res << " <- ";
list<string> pstrs;
  for(int i=0; i<nParams; i++)
    pstrs.push_back(paramNames[i] + ":" + params[i]->str());
  res << joinStrings(pstrs,',');
  return res.str();
}

Function::Function(FuncType &outside, SymbolTable *where)
{
  type = where->getCanon(outside);
  scope = new SymbolTable(where);
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

const DataType *SymbolTable::lookupSymbol(string name) const
{
map<string,const DataType*>::const_iterator it = symbols.find(name);
  if(it!=symbols.end())
    return it->second;
  if(parent!=NULL)
    return parent->lookupSymbol(name);
  return NULL;
}

const DataType *SymbolTable::lookupTypedef(string name) const
{
map<string,const DataType*>::const_iterator it = typedefs.find(name);
  if(it!=typedefs.end())
    return it->second;
  if(parent!=NULL)
    return parent->lookupTypedef(name);
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
void SymbolTable::dump()
{
map<string,TypeAtom>::iterator it;
  for(it=symbols.begin(); it!=symbols.end(); ++it)
    printf("Decl: %s -> %s\n", it->first.c_str(), it->second.str().c_str());
  for(it=typedefs.begin(); it!=typedefs.end(); ++it)
    printf("Type: %s -> %s\n", it->first.c_str(), it->second.str().c_str());
map<string,list<Decl> >::iterator recIt;    
  for(recIt=tags.begin(); recIt!=tags.end(); ++recIt)
  {
    printf("Tag: %s -> {", recIt->first.c_str());
    for(list<Decl>::iterator d=recIt->second.begin(); d!=recIt->second.end(); ++d)
      printf("%s:%s;", d->name.c_str(), d->type.str().c_str());
    printf("}\n");
  }
/*
map<string,Function *>::iterator fit;
  for(fit=functions.begin(); fit!=functions.end(); ++fit)
    printf("Function: %s -> %s\n", fit->first.c_str(), fit->second->type->str().c_str());
    */
}

bool SymbolTable::validTypedef(string name)
{
  return typedefs.find(name)!=typedefs.end();
}

TypeAtom SymbolTable::getTypedef(string name) 
{
  return typedefs[name];
}

void SymbolTable::saveRecord(string name, list<Decl> &fields)
{
  printf("SAVING %s %u\n",name.c_str(), fields.size());
  tags[name] = fields;    // Should overwrite forward refs
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

