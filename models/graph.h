#include<set>
#include<list>
#include<map>
/* Assume that both Node and Edge can be passed-by-value / shallow copied.
   Assume that all DiGraphs are monotonically constructed (no deletion).
   Assume that values can be ordered for set storage.
*/
template<class Node, class Edge>
class DiGraph
{
  std::map<Node, std::set< std::pair<Node,Edge> > > storage;
public:
  void add(Node const &n);
  void add(Node const &src, Node const &tar, Edge const &e);
  std::set<Node> next(Node const &src) const; 
  std::set<Edge> outgoing(Node const &src) const;
  typedef std::pair<Node,Node> NodePair;
  typedef std::pair<Edge,NodePair> Triple;
  typedef std::pair<Node,Edge> Link;
  std::list<Triple> edges() const;
  std::set<Node> nodes() const;
  std::set<Node> sources() const;
  std::set<Node> sinks() const;
  std::set<Node> reachable(Node start) const;
  DiGraph<Node,Edge> flip() const;
  std::list<Node> topSort(std::set<Node> ready) const;
  std::list<Node> bfs(Node start) const;
  std::list<Node> dfs(Node start) const;
};
