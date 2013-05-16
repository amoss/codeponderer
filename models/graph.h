
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

  typedef std::pair<Node,Node> NodePair;
  typedef std::pair<Edge,NodePair> Triple;
  typedef std::pair<Node,Edge> Link;
  std::list<Triple> edges() const
  {
    std::list<Triple> result;
    for(typename std::map<Node,std::set<Link> >::const_iterator 
        it = storage.begin(); it!=storage.end(); ++it)
    {
      for(typename std::set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
        result.push_back( Triple(es->second, NodePair(it->first,es->first)));
    }
    return result;
  }

  std::set<Node> nodes() const
  {
    std::set<Node> result;
    for(typename std::map<Node,std::set<Link> >::const_iterator 
        it = storage.begin(); it!=storage.end(); ++it)
    {
      result.insert(it->first);
      for(typename std::set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
        result.insert(es->first);
    }
    return result;
  }

  std::set<Node> sources() const
  {
    std::set<Node> result = nodes();
    for(typename std::map<Node,std::set<Link> >::const_iterator 
        it = storage.begin(); it!=storage.end(); ++it)
    {
      for(typename std::set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
        result.erase(es->first);
    }
    return result;
  }

  std::set<Node> sinks() const
  {
    std::set<Node> result = nodes();
    for(typename std::map<Node,std::set<Link> >::const_iterator 
        it = storage.begin(); it!=storage.end(); ++it)
    {
      for(typename std::set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
        result.insert(es->first);
    }
    for(typename std::map<Node,std::set<Link> >::const_iterator 
        it = storage.begin(); it!=storage.end(); ++it)
    {
      if(it->second.size()>0)
        result.erase(it->first);
    }
    return result;
  }

  std::set<Node> reachable(Node start) const
  {
    std::set<Node> result, frontier;
    frontier.insert(start);
    while(frontier.size()>0)
    {
      // Pop an arbitrary Node from the frontier (don't remove until after processing
      // the outgoing links to handle cyclic pairs correctly).
      typename std::set<Node>::iterator pos = frontier.begin();
      Node cur = *pos;
      result.insert(cur);

      // Get the set of descendent Nodes
      typename std::map<Node,std::set<Link> >::const_iterator ds = storage.find(cur);
      if( ds != storage.end() )
      {
        typename std::set<Link>::iterator outg = ds->second.begin();
        for(; outg!=ds->second.end(); ++outg)
          if( result.find(outg->first)==result.end() )
            frontier.insert(outg->first);
      }

      // Finish the pop
      frontier.erase(pos);
    }
    return result;
  }

  // TODO: Flip   -> build a copy of the graph with edges inverted
  //       Induce -> build a copy of the induced subgraph
  //       Invert -> build a copy of the induced region not in the subgraph
  //       SCC    -> strongly connected components

  std::list<Node> bfs(Node start) const
  {
    std::list<Node> queue, ordering;
    queue.push_back(start);
    while(queue.size()>0)
    {
      Node cur = *queue.back();
      // TODO
    }
  }

  std::list<Node> dfs(Node start) const
  {
    std::list<Node> queue, ordering;
    queue.push_back(start);
    while(queue.size()>0)
    {
      Node cur = *queue.back();
      // TODO
    }
  }
};
