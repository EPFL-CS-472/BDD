#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

/* These are just some hacks to hash std::pair (for the unique table).
 * You don't need to understand this part. */
namespace std
{
template<class T>
inline void hash_combine( size_t& seed, T const& v )
{
  seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
struct hash<pair<uint32_t, uint32_t>>
{
  using argument_type = pair<uint32_t, uint32_t>;
  using result_type = size_t;
  result_type operator() ( argument_type const& in ) const
  {
    result_type seed = 0;
    hash_combine( seed, in.first );
    hash_combine( seed, in.second );
    return seed;
  }
};
}

class BDD
{
public:
  using index_t = uint32_t;
  /* Declaring `index_t` as an alias for an unsigned integer.
   * This is just for easier understanding of the code.
   * This datatype will be used for node indices. */

  using var_t = uint32_t;
  /* Similarly, declare `var_t` also as an alias for an unsigned integer.
   * This datatype will be used for representing variables. */

public:
  struct Edge
  {
    bool inv;
    index_t child;

    bool operator==(Edge r)
    {
      return this->inv == r.inv && this->child == r.child;
    }


  };
  struct Node
  {
    var_t v; /* corresponding variable */
    Edge T; /* index of THEN child */
    Edge E; /* index of ELSE child */
  };
  struct VectorHasher {
      int operator()(const std::vector<index_t> &V) const {
          int hash = V.size();
          for(auto &i : V) {
              hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
          }
          return hash;
      }
  };



public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_not( 0u ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back( Node({num_vars, {.inv=false,.child=0}, {.inv=false,.child=0}}) ); /* constant 0 */
  //  ref_count.at(nodes.size()) = 0;
    nodes.emplace_back( Node({num_vars, {.inv=false,.child=1}, {.inv=false,.child=1}}) ); /* constant 1 */
  //  ref_count.at(nodes.size()) = 0;
    /* `nodes` is initialized with two `Node`s representing the terminal (constant) nodes.
     * Their `v` is `num_vars` and their indices are 0 and 1.
     * (Note that the real variables range from 0 to `num_vars - 1`.)
     * Both of their children point to themselves, just for convenient representation.
     *
     * `unique_table` is initialized with `num_vars` empty maps. */
  }


  /**********************************************************/
  /***************** Basic Building Blocks ******************/
  /**********************************************************/

  uint32_t num_vars() const
  {
    return unique_table.size();
  }

  /* Get the (index of) constant node. */
  index_t constant( bool value ) const
  {
    return value ? 1 : 0;
  }
  /* TODO : Avoid recounting at each addition (if possible) */
  void count_references()
  {
       for(auto i = 2u; i < nodes.size(); ++i)
       {
           ref_count[i] = 0;
       }
       for(auto i= 2u; i < nodes.size(); ++i)
       {
        ref_count[nodes.at(i).T.child]++;
        ref_count[nodes.at(i).E.child]++;
       }
      /* for(auto i = 2u; i < nodes.size(); ++i)
       {
           std::cout << "Node " << i << " has refs : " << ref_count[i] << std::endl;
       }*/

  }

  index_t lookup_computed_table(index_t then, index_t els)
  {
      auto ret = computed_table.find({then, els});
     // std::cout << "lct" << then << "  " << els << std::endl;
      if (ret != computed_table.end()){
        //  std::cout << ret->first.first << "   " << ret->first.second << "    " << ret->second << std::endl;
          return ret->second;
      }
      else{
      //    std::cout << "LCT NULL" << std::endl;
          return NULL;
      }
  }

  void computed_table_insert(std::vector<index_t> ops, index_t ret)
  {
     // std::cout << "CTI T : " << then << " else :  " << els << "  ret :  " << ret << std::endl;
      computed_table[ops] = ret;
  }

  bool invert_edges_of_node(index_t node){
      bool ret = false;
      for(auto i = 0u; i < nodes.size(); ++i){
          if (i == node)
              nodes.at(i).E.inv = true;
          if(nodes.at(i).E.child == node){
              nodes.at(i).E.inv = !nodes.at(i).E.inv;
              ret = true;
          }
          if(nodes.at(i).T.child == node){
              nodes.at(i).T.inv = !nodes.at(i).T.inv;
              ret = true;
          }
      }
      return ret;
  }

  bool checkChildren(Node n){
      bool ret = true;
      Edge e = n.E;
      Edge t = n.T;
      ret &= (nodes.at(e.child).E == nodes.at(t.child).E);
      ret &= (nodes.at(e.child).T == nodes.at(t.child).T);
      return ret;
  }

  void cce_conversion(Edge f)
  {
      std::cout << "---- Before CCE conversion --- " << std::endl;
      print(f);

      // 1. mark all "Else" edges (no need)
      // 2. Remove 1 Leafs (maybe not yet)
      // 3. Invert all edges leading to 0
      for(auto i = 0u; i < nodes.size(); ++i)
      {
          if(nodes.at(i).E.child == 0){
              nodes.at(i).E.child = 1;
              nodes.at(i).E.inv = true;
          }else if (nodes.at(i).T.child == 0){
              nodes.at(i).T.child = 1;
              nodes.at(i).T.inv = true;
          }
      }
      // 4. Remove inversion from all "Then" edges by inverting all other edges from/to the Node
      bool edge_modified = true;
       print(f);
      while(edge_modified){
          edge_modified = false;
          std::cout << "---- New iteration --- " << std::endl;
          for(auto i = 0u; i < nodes.size(); ++i){
              if(nodes.at(i).T.inv){
                 nodes.at(i).T.inv = false;
                 std::cout << "modified node " << i << std::endl;
                 edge_modified |= invert_edges_of_node(i);
                 print(f);

              }
          }
      }
      for(auto i = 0u; i < nodes.size(); ++i){
          if(checkChildren(nodes.at(i))){
            nodes.at(i).E.child = nodes.at(i).T.child;
          }
      }
      std::cout << "---- After CCE conversion --- " << std::endl;
      print(f);

  }
  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  Edge unique( var_t var, Edge T, Edge E )
  {
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( T.child < nodes.size() && "Make sure the children exist." );
    assert( E.child < nodes.size() && "Make sure the children exist." );
    assert( nodes[T.child].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[E.child].v > var && "With static variable order, children can only be below the node." );

    /* Reduction rule: Identical children */
    if ( T == E )
    {
      return T;
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T.child, E.child} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */
      /* TO FIX Either return node or edge but might be an issue" */
      return {0, it->second};
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      index_t const new_index = nodes.size();
      nodes.emplace_back( Node({var, T, E}) );
   //   ref_count.at(new_index) = 0;
      unique_table[var][{T.child, E.child}] = new_index;
      count_references();
      return {0, new_index};
    }
  }

  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  Edge literal( var_t var, bool complement = false )
  {
    return unique( var, {.inv=false, .child=constant( !complement )}, {.inv=false, .child=constant( complement )} );
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
  Edge NOT( Edge f )
  {
    assert( f.child < nodes.size() && "Make sure f exists." );
    Edge ret = {0, lookup_computed_table(f.child, f.child)};
    if(ret.child)
        return ret;
    ++num_invoke_not;

    /* trivial cases */
    if ( f.child == constant( false ) )
    {
      return {0, constant( true )};
    }
    if ( f.child == constant( true ) )
    {
      return {0, constant( false )};
    }

    Node const& F = nodes[f.child];
    var_t x = F.v;
    Edge f0 = F.E, f1 = F.T;

    Edge const r0 = NOT( f0 );
    Edge const r1 = NOT( f1 );
    Edge r = unique( x, r1, r0 );
    std::vector<index_t> ops = {f.child};
    computed_table_insert(ops, r.child);
    return r;

  }

  /* Compute f ^ g */
  Edge XOR( Edge f, Edge g )
  {
    assert( f.child < nodes.size() && "Make sure f exists." );
    assert( g.child < nodes.size() && "Make sure g exists." );
    Edge ret = {0, lookup_computed_table(f.child, g.child)};
    if(ret.child)
        return ret;
    ++num_invoke_xor;

    /* trivial cases */
    if ( f == g )
    {
      return {0, constant( false )};
    }
    if ( f.child == constant( false ) )
    {
      return g;
    }
    if ( g.child == constant( false ) )
    {
      return f;
    }
    if ( f.child == constant( true ) )
    {
      return NOT( g );
    }
    if ( g.child == constant( true ) )
    {
      return NOT( f );
    }
    if ( f == NOT( g ) )
    {
      return {0, constant( true )};
    }

    Node const& F = nodes[f.child];
    Node const& G = nodes[g.child];
    var_t x;
    Edge f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = G.E;
      g1 = G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = G.E;
      g1 = G.T;
    }

    Edge const r0 = XOR( f0, g0 );
    Edge const r1 = XOR( f1, g1 );
    Edge r = unique( x, r1, r0 );
    std::vector<index_t> ops = {f.child, g.child};
    computed_table_insert(ops, r.child);
    return r;
  }

  /* Compute f & g */
  Edge AND( Edge f, Edge g )
  {
    std::cout << f.child << std::endl;
    assert( f.child < nodes.size() && "Make sure f exists." );
    assert( g.child < nodes.size() && "Make sure g exists." );
    Edge ret = {0, lookup_computed_table(f.child, g.child)};
    if(ret.child)
        return ret;
    ++num_invoke_and;

    /* trivial cases */
    if ( f.child == constant( false ) || g.child == constant( false ) )
    {
      return {0, constant( false )};
    }
    if ( f.child == constant( true ) )
    {
      return g;
    }
    if ( g.child == constant( true ) )
    {
      return f;
    }
    if ( f == g )
    {
      return f;
    }

    Node const& F = nodes[f.child];
    Node const& G = nodes[g.child];
    var_t x;
    Edge f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = G.E;
      g1 = G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = G.E;
      g1 = G.T;
    }

    Edge const r0 = AND( f0, g0 );
    Edge const r1 = AND( f1, g1 );
    Edge r =  unique( x, r1, r0 );
    std::vector<index_t> ops = {f.child, g.child};
    computed_table_insert(ops, r.child);
    return r;
  }

  /* Compute f | g */
  Edge OR( Edge f, Edge g )
  {
    assert( f.child < nodes.size() && "Make sure f exists." );
    assert( g.child < nodes.size() && "Make sure g exists." );
    Edge ret = {0, lookup_computed_table(f.child, g.child)};
    if(ret.child)
        return ret;
    ++num_invoke_or;

    /* trivial cases */
    if ( f.child == constant( true ) || g.child == constant( true ) )
    {
      return {0, constant( true )};
    }
    if ( f.child == constant( false ) )
    {
      return g;
    }
    if ( g.child == constant( false ) )
    {
      return f;
    }
    if ( f == g )
    {
      return f;
    }

    Node const& F = nodes[f.child];
    Node const& G = nodes[g.child];
    var_t x;
    Edge f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = G.E;
      g1 = G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      g0 = G.E;
      g1 = G.T;
    }

    Edge const r0 = OR( f0, g0 );
    Edge const r1 = OR( f1, g1 );
    Edge r = unique( x, r1, r0 );
    std::vector<index_t> ops = {f.child, g.child};
    computed_table_insert(ops, r.child);
    return r;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  Edge ITE( Edge f, Edge g, Edge h )
  {
    assert( f.child < nodes.size() && "Make sure f exists." );
    assert( g.child < nodes.size() && "Make sure g exists." );
    assert( h.child < nodes.size() && "Make sure h exists." );
    Edge ret = {0, lookup_computed_table(g.child, h.child)};
    if(ret.child)
        return ret;
    ++num_invoke_ite;

    /* trivial cases */
    if ( f.child == constant( true ) )
    {
      return g;
    }
    if ( f.child == constant( false ) )
    {
      return h;
    }
    if ( g == h )
    {
      return g;
    }

    Node const& F = nodes[f.child];
    Node const& G = nodes[g.child];
    Node const& H = nodes[h.child];
    var_t x;
    Edge f0, f1, g0, g1, h0, h1;
    if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
    {
      x = F.v;
      f0 = F.E;
      f1 = F.T;
      if ( G.v == F.v )
      {
        g0 = G.E;
        g1 = G.T;
      }
      else
      {
        g0 = g1 = g;
      }
      if ( H.v == F.v )
      {
        h0 = H.E;
        h1 = H.T;
      }
      else
      {
        h0 = h1 = h;
      }
    }
    else /* F.v > min(G.v, H.v) */
    {
      f0 = f1 = f;
      if ( G.v < H.v )
      {
        x = G.v;
        g0 = G.E;
        g1 = G.T;
        h0 = h1 = h;
      }
      else if ( H.v < G.v )
      {
        x = H.v;
        g0 = g1 = g;
        h0 = H.E;
        h1 = H.T;
      }
      else /* G.v == H.v */
      {
        x = G.v;
        g0 = G.E;
        g1 = G.T;
        h0 = H.E;
        h1 = H.T;
      }
    }

    Edge const r0 = ITE( f0, g0, h0 );
    Edge const r1 = ITE( f1, g1, h1 );
    Edge r = unique( x, r1, r0 );
    std::vector<index_t> ops = {f.child, g.child, h.child};
    computed_table_insert(ops, r.child);
    return r;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( Edge f, std::ostream& os = std::cout ) const
  {
    for ( auto i = 0u; i < nodes[f.child].v; ++i )
    {
      os << "  ";
    }
    if ( f.child <= 1 )
    {
      os << "node " << f.child << ": constant " << f.child << "  inv : " << f.inv << std::endl;
    }
    else
    {
      os << "node " << f.child << ": var = " << nodes[f.child].v << ", T = " << nodes[f.child].T.child << "inv : " << nodes[f.child].T.inv
         << ", E = " << nodes[f.child].E.child << "inv : " << nodes[f.child].E.inv << std::endl;
      for ( auto i = 0u; i < nodes[f.child].v; ++i )
      {
        os << "  ";
      }
      os << "> THEN branch" << std::endl;
      print( nodes[f.child].T, os );
      for ( auto i = 0u; i < nodes[f.child].v; ++i )
      {
        os << "  ";
      }
      os << "> ELSE branch" << std::endl;
      print( nodes[f.child].E, os );
    }
  }

  /* Get the truth table of the BDD rooted at node f. */
  Truth_Table get_tt( Edge f ) const
  {
   // std::cout << "get tt" << f.child << std::endl;
    assert( f.child < nodes.size() && "Make sure f exists." );
    assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );

    if ( f.child == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f.child == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    var_t const x = nodes[f.child].v;
    Edge const fx = nodes[f.child].T;
    Edge const fnx = nodes[f.child].E;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    return ( tt_x & get_tt( fx ) ) | ( tt_nx & get_tt( fnx ) );
  }

  /* Whether `f` is dead (having a reference count of 0). */
  bool is_dead( index_t f ) const
  {

    return ref_count.at(f) == 0;
  }




  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 2u; i < nodes.size(); ++i )
    {
      if ( !is_dead( i ) )
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( Edge f ) const
  {
    assert( f.child < nodes.size() && "Make sure f exists." );

    if ( f.child == constant( false ) || f.child == constant( true ) )
    {
      return 0u;
    }

    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    visited[1] = true;

    return num_nodes_rec( f, visited );
  }

  uint64_t num_invoke() const
  {
    return num_invoke_not + num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
  }

//private:
  /**********************************************************/
  /******************** Helper Functions ********************/
  /**********************************************************/

  uint64_t num_nodes_rec( Edge f, std::vector<bool>& visited ) const
  {
    assert( f.child < nodes.size() && "Make sure f exists." );
    std::cout << "Reaching Node " << f.child << std::endl;

    uint64_t n = 0u;
    Node const& F = nodes[f.child];
    assert( F.T.child < nodes.size() && "Make sure the children exist." );
    assert( F.E.child < nodes.size() && "Make sure the children exist." );
    if ( !visited[F.T.child] )
    {
      n += num_nodes_rec( F.T, visited );
      visited[F.T.child] = true;
    }
    if ( !visited[F.E.child] )
    {
      n += num_nodes_rec( F.E, visited );
      visited[F.E.child] = true;
    }
    return n + 1u;
  }

private:
  std::vector<Node> nodes;
  std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table;
  std::unordered_map<index_t,uint32_t> ref_count;
  std::unordered_map<std::vector<index_t>, index_t,VectorHasher> computed_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */

  /* statistics */
  uint64_t num_invoke_not, num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
};
