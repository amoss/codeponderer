
/* Assume that both Node and Edge can be passed-by-value / shallow copied.
   Assume that all DiGraphs are monotonically constructed (no deletion).
   Assume that values can be ordered for set storage.
*/
template<class Node, class Edge>
class DiGraph
{
  std::map<Node, std::set< std::pair<Node,Edge> > > storage;
public:
  void add(Node const &n)
  {
    if( storage.find(n)==storage.end() )
      storage[n] = std::set< std::pair<Node,Edge> >();
  }
  void add(Node const &src, Node const &tar, Edge const &e)
  {
    add(src);
    add(tar);
    storage[src].insert( std::pair<Node,Edge>(tar,e) );
  }

  std::set<Node> next(Node const &src)
  {
    std::set<Node> result;
    for(typename std::set< std::pair<Node,Edge> >::iterator it = storage[src].begin(); it!=storage[src].end(); ++it)
      result.insert( it->first );
    return result;
  }

  std::set<Edge> outgoing(Node const &src)
  {
    std::set<Node> result;
    for(typename std::set< std::pair<Node,Edge> >::iterator it = storage[src].begin(); it!=storage[src].end(); ++it)
      result.insert( it->second );
    return result;
  }
};
