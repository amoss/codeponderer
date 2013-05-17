// Meant to be directly included into using modules - no headers, inside std namespace

template<class Node, class Edge>
void DiGraph<Node,Edge>::add(Node const &n)
{
  if( storage.find(n)==storage.end() )
    storage[n] = set< pair<Node,Edge> >();
}

template<class Node, class Edge>
void DiGraph<Node,Edge>::add(Node const &src, Node const &tar, Edge const &e)
{
  add(src);
  add(tar);
  storage[src].insert( pair<Node,Edge>(tar,e) );
}

template<class Node, class Edge>
set<Node> DiGraph<Node,Edge>::next(Node const &src) const
{
  set<Node> result;
  set< DiGraph<Node,Edge>::Link > const &lookup = storage.at(src);
  for(typename set< pair<Node,Edge> >::const_iterator it = lookup.begin(); it!=lookup.end(); ++it)
    result.insert( it->first );
  return result;
}

template<class Node, class Edge>
set<Edge> DiGraph<Node,Edge>::outgoing(Node const &src) const
{
  set<Node> result;
  for(typename set< pair<Node,Edge> >::const_iterator it = storage[src].begin(); it!=storage[src].end(); ++it)
    result.insert( it->second );
  return result;
}

template<class Node, class Edge>
list<typename DiGraph<Node,Edge>::Triple> DiGraph<Node,Edge>::edges() const
{
  list<Triple> result;
  for(typename map<Node,set<Link> >::const_iterator 
      it = storage.begin(); it!=storage.end(); ++it)
  {
    for(typename set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
      result.push_back( Triple(es->second, NodePair(it->first,es->first)));
  }
  return result;
}

template<class Node, class Edge>
set<Node> DiGraph<Node,Edge>::nodes() const
{
  set<Node> result;
  for(typename map<Node,set<Link> >::const_iterator 
      it = storage.begin(); it!=storage.end(); ++it)
  {
    result.insert(it->first);
    for(typename set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
      result.insert(es->first);
  }
  return result;
}

template<class Node, class Edge>
set<Node> DiGraph<Node,Edge>::sources() const
{
  set<Node> result = nodes();
  for(typename map<Node,set<Link> >::const_iterator 
      it = storage.begin(); it!=storage.end(); ++it)
  {
    for(typename set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
      result.erase(es->first);
  }
  return result;
}

template<class Node, class Edge>
set<Node> DiGraph<Node,Edge>::sinks() const
{
  set<Node> result = nodes();
  for(typename map<Node,set<Link> >::const_iterator 
      it = storage.begin(); it!=storage.end(); ++it)
  {
    for(typename set<Link>::iterator es=it->second.begin(); es!=it->second.end(); ++es)
      result.insert(es->first);
  }
  for(typename map<Node,set<Link> >::const_iterator 
      it = storage.begin(); it!=storage.end(); ++it)
  {
    if(it->second.size()>0)
      result.erase(it->first);
  }
  return result;
}

template<class Node, class Edge>
set<Node> DiGraph<Node,Edge>::reachable(Node start) const
{
  set<Node> result, frontier;
  frontier.insert(start);
  while(frontier.size()>0)
  {
    // Pop an arbitrary Node from the frontier (don't remove until after processing
    // the outgoing links to handle cyclic pairs correctly).
    typename set<Node>::iterator pos = frontier.begin();
    Node cur = *pos;
    result.insert(cur);

    // Get the set of descendent Nodes
    typename map<Node,set<Link> >::const_iterator ds = storage.find(cur);
    if( ds != storage.end() )
    {
      typename set<Link>::iterator outg = ds->second.begin();
      for(; outg!=ds->second.end(); ++outg)
        if( result.find(outg->first)==result.end() )
          frontier.insert(outg->first);
    }

    // Finish the pop
    frontier.erase(pos);
  }
  return result;
}

// build a copy of the graph with edge directions inverted
template<class Node, class Edge>
DiGraph<Node,Edge> DiGraph<Node,Edge>::flip() const
{
  DiGraph<Node,Edge> result;
  typedef DiGraph<Node,Edge>::Triple Triple;
  list<Triple> trips = edges();
  for(typename list<Triple>::iterator t=trips.begin(); t!=trips.end(); ++t)
    result.add(t->second.second, t->second.first, t->first);
  return result;
}

template<class Node, class Edge>
list<Node> DiGraph<Node,Edge>::topSort(set<Node> ready) const
{
  list<Node> result;
  while(ready.size()>0)
  {
    typename set<Node>::iterator pos = ready.begin();
    Node cur = *pos;
    result.push_back(cur);

    set<Node> front = next(cur);
    printf("%u\n",front.size());
    tmplForeach(set, Node, n, front)
      printf("n=%s\n",n.str().c_str());
    tmplEnd
    // For every descendent of cur
      // For every parent of node
        // If parent is not ready or result then node is blocked
    // If node is not blocked then add to ready
    break;
  }
  return result;
}

//       Induce -> build a copy of the induced subgraph
//       Invert -> build a copy of the induced region not in the subgraph
//       SCC    -> strongly connected components

template<class Node, class Edge>
list<Node> DiGraph<Node,Edge>::bfs(Node start) const
{
  list<Node> queue, ordering;
  queue.push_back(start);
  while(queue.size()>0)
  {
    Node cur = *queue.back();
    // TODO
  }
}

template<class Node, class Edge>
list<Node> DiGraph<Node,Edge>::dfs(Node start) const
{
  list<Node> queue, ordering;
  queue.push_back(start);
  while(queue.size()>0)
  {
    Node cur = *queue.back();
    // TODO
  }
}
