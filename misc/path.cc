#include<sstream>
#include "misc/path.h"

using namespace std;

static list<string> splitPath(string const &path)
{
list<string> components;
size_t pos = 0;
  while( true )
  {
    size_t next = path.find('/',pos);
    if( next!=string::npos )
    {
      components.push_back(path.substr(pos,next-pos));  // drops slashes from length
      pos = next+1;
    }
    else
    {
      components.push_back(path.substr(pos));
      break;
    }
  }
  return components;
}

Path::Path(string const &p)
{
  ups = 0;
size_t pos=0;
  // Absolute or relative path
  if(p.at(pos)=='/') {
    relative = false;
    components = splitPath(p.substr(1));
  }
  else
  {
    relative = true;
    components = splitPath(p);
  }

  for(list<string>::iterator c=components.begin(); c!=components.end();)
  {
    if(*c == ".")
      c = components.erase(c);
    else if(*c == "..")
    {
      if(c==components.begin())
        ups++;
      else
        c = components.erase(--c);
      c = components.erase(c);
    }
    else
      ++c;
  }
}

bool Path::inside(const Path &above) const
{
  if((relative && !above.relative) ||
     (!relative && above.relative))
    return false;                 // Cannot define containment on mixed paths
  if( (relative && above.relative) && ups!=above.ups) // Otherwise depends on tree 
    return false;
  if(above.components.size() > components.size())
    return false;
  list<string>::const_iterator c1=components.begin(),c2=above.components.begin();
  for( ;c2!=above.components.end(); ++c1,++c2)
    if(*c1 != *c2)
      return false;
  return true;
}

string Path::repr() const
{
stringstream result;
  if(!relative)
    result << "/";
  for(int i=0; i<ups; i++)
    result << "../";
  for( list<string>::const_iterator c=components.begin(); c!=components.end();)
  {
    result << *c;
    if( ++c != components.end() )
      result << '/';
  }
  return result.str();
}
