#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

#include <limits>

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

template<>
struct hash<tuple<uint32_t, uint32_t, uint32_t>>
{
  using argument_type = tuple<uint32_t, uint32_t, uint32_t>;
  using result_type = size_t;
  result_type operator() ( argument_type const& in ) const
  {
    result_type seed = 0;
    hash_combine( seed, std::get<0>(in));
    hash_combine( seed, std::get<1>(in));
    hash_combine( seed, std::get<2>(in));
    return seed;
  }
};
}

class BDD
{
public:

  using value_t = uint32_t; // pointer to Edge

  using index_t = uint32_t; // pointer to Node

  using var_t = uint32_t;
  /* Similarly, declare `var_t` also as an alias for an unsigned integer.
   * This datatype will be used for representing variables. */

private:
  struct Node
  {
    var_t v; /* corresponding variable */
    value_t T; /* index of THEN child */
    value_t E; /* index of ELSE child */

    value_t pos_ref; // positive parent, num_vars means empty
    value_t neg_ref; // negative parent, num_vars means empty

    uint32_t reference_count;
  };

  struct NodeReference {
    bool flipped;
    index_t node_idx;
  };

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u ), repeat_invoke(0u)
  {
    nodes.emplace_back(Node({num_vars, 1, 1, 1, 0, 0})); /* constant 1 */
    refs.emplace_back(NodeReference{
      true, 0
    }); // constant 0
    refs.emplace_back(NodeReference{
      false, 0
    }); // constant 1
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
  value_t constant( bool value ) const
  {
    return value ? 1 : 0;
  }

  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  index_t unique( var_t var, value_t T, value_t E )
  {
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( T < refs.size() && "Make sure the children exist." );
    assert( E < refs.size() && "Make sure the children exist." );
    NodeReference &rT = refs[T];
    NodeReference &rE = refs[E];
    assert( nodes[rT.node_idx].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[rE.node_idx].v > var && "With static variable order, children can only be below the node." );

    /* Reduction rule: Identical children */
    if ( T == E )
    {
      return ref(T);
    }
    
    bool result_flipped = false;
    // what if T.flipped is true?
    if(rT.flipped){
      result_flipped = true;
      T = buildReference(false, rT.node_idx);
      E = getFlippedReference(E);
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T, E} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */
      //repeat_invoke += 1;
      return buildReference(result_flipped, it->second);
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      ref(T);
      ref(E);
      index_t const new_index = nodes.size();
      nodes.emplace_back( Node({var, T, E, std::numeric_limits<index_t>::max(), std::numeric_limits<index_t>::max(), 0}) );
      unique_table[var][{T, E}] = new_index;
      return buildReference(result_flipped, new_index);
    }
  }

  index_t ref(index_t other){
    NodeReference &rO = refs[other];
    Node &nO = nodes[rO.node_idx];
    ++nO.reference_count;
    return other;
  }

  void deref(index_t other){
    if(other == 0 || other == 1) return; 
    NodeReference &rO = refs[other];
    Node &nO = nodes[rO.node_idx];
    assert(nO.reference_count > 0 && "It's impossible to delete a dead node!");
    --nO.reference_count;
    if(nO.reference_count == 0){
      // dead code
      deref(nO.T);
      deref(nO.E);
    }
  }

  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  value_t literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ));
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
  value_t NOT( value_t f )
  {
    // assert( f < nodes.size() && "Make sure f exists." );

    // /* trivial cases */
    // if ( f == constant( false ) )
    // {
    //   return constant( true );
    // }
    // if ( f == constant( true ) )
    // {
    //   return constant( false );
    // }

    // Node const& F = nodes[f];
    // var_t x = F.v;
    // index_t f0 = F.E, f1 = F.T;

    // index_t const r0 = NOT( f0 );
    // index_t const r1 = NOT( f1 );
    return getFlippedReference(f);
  }

  /* Compute f ^ g */
  value_t XOR( value_t f, value_t g )
  {
    assert( f < refs.size() && "Make sure f exists." );
    assert( g < refs.size() && "Make sure g exists." );

    ++num_invoke_xor;

    value_t res = 0;

    /* trivial cases */
    if ( f == g )
    {
      res = constant( false );
    } else if ( f == constant( false ) ){
      res = g;
    } else if ( g == constant( false ) ){
      res = f;
    } else if ( f == constant( true ) ){
      res = NOT( g );
    } else if ( g == constant( true ) ){
      res = NOT( f );
    } else if ( f == NOT( g ) ){
      res = constant( true );
    } else {
      NodeReference rF = refs[f];
      NodeReference rG = refs[g];

      Node const& F = nodes[rF.node_idx];
      Node const& G = nodes[rG.node_idx];

      var_t x;
      value_t f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }

      value_t r0, r1;

      auto r0_lookup = lookUpBinaryComputedTable(xor_uni, f0, g0);
      if(r0_lookup.first){
        r0 = r0_lookup.second;
      } else {
        r0 = XOR( f0, g0 );
      }

      auto r1_lookup = lookUpBinaryComputedTable(xor_uni, f1, g1);
      if(r1_lookup.first){
        r1 = r1_lookup.second;
      } else {
        r1 = XOR(f1, g1);
      }
      res = unique(x, r1, r0);
    }

    registerToBinaryComputedTable(xor_uni, f, g, res);
    registerToBinaryComputedTable(xor_uni, getFlippedReference(f), g, getFlippedReference(res));
    registerToBinaryComputedTable(xor_uni, f, getFlippedReference(g), getFlippedReference(res));
    registerToBinaryComputedTable(xor_uni, getFlippedReference(f), getFlippedReference(g), res);

    return res;
  }

  /* Compute f & g */
  value_t AND( value_t f, value_t g )
  {
    assert( f < refs.size() && "Make sure f exists." );
    assert( g < refs.size() && "Make sure g exists." );
    ++num_invoke_and;

    value_t res = 0;

    /* trivial cases */
    if ( f == constant( false ) || g == constant( false ) ){
      res = constant( false );
    } else if ( f == constant( true ) ){
      res = g;
    } else if ( g == constant( true ) ){
      res = f;
    } else if ( f == g ){
      res = f;
    } else {
      NodeReference rF = refs[f];
      NodeReference rG = refs[g];

      Node const& F = nodes[rF.node_idx];
      Node const& G = nodes[rG.node_idx];

      var_t x;
      value_t f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }

      value_t r0, r1;
      auto look_r0 = lookUpBinaryComputedTable(and_uni, f0, g0);
      if(look_r0.first){
        r0 = look_r0.second;
      } else {
        r0 = AND( f0, g0 );
      }
      auto look_r1 = lookUpBinaryComputedTable(and_uni, f1, g1);
      if(look_r1.first){
        r1 = look_r1.second;
      } else {
        r1 = AND( f1, g1 );
      }
      res = unique(x, r1, r0);
    }

    registerToBinaryComputedTable(and_uni, f, g, res);
    registerToBinaryComputedTable(or_uni, getFlippedReference(f), getFlippedReference(g), getFlippedReference(res));

    return res;
  }

  /* Compute f | g */
  value_t OR( value_t f, value_t g )
  {
    assert( f < refs.size() && "Make sure f exists." );
    assert( g < refs.size() && "Make sure g exists." );
    ++num_invoke_or;

    value_t res = 0;

    /* trivial cases */
    if ( f == constant( true ) || g == constant( true ) ){
      res = constant( true );
    } else if ( f == constant( false ) ){
      res = g;
    } else if ( g == constant( false ) ){
      res = f;
    } else if ( f == g ) {
      res = f;
    } else {
      NodeReference rF = refs[f];
      NodeReference rG = refs[g];

      Node const& F = nodes[rF.node_idx];
      Node const& G = nodes[rG.node_idx];

      var_t x;
      value_t f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
        g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
      }

      value_t r0, r1;
      auto look_r0 = lookUpBinaryComputedTable(or_uni, f0, g0);
      if(look_r0.first){
        r0 = look_r0.second;
      } else {
        r0 = OR( f0, g0 );
      }
      auto look_r1 = lookUpBinaryComputedTable(or_uni, f1, g1);
      if(look_r1.first){
        r1 = look_r1.second;
      } else {
        r1 = OR( f1, g1 );
      }
      res = unique(x, r1, r0);
    }

    registerToBinaryComputedTable(or_uni, f, g, res);
    registerToBinaryComputedTable(and_uni, getFlippedReference(f), getFlippedReference(g), getFlippedReference(res));

    return res;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  value_t ITE( value_t f, value_t g, value_t h )
  {
    assert( f < refs.size() && "Make sure f exists." );
    assert( g < refs.size() && "Make sure g exists." );
    assert( h < refs.size() && "Make sure h exists." );
    ++num_invoke_ite;

    value_t res = 0;
    
    /* trivial cases */
    if ( f == constant( true ) ){
      res = g;
    } else if ( f == constant( false ) ){
      res = h;
    } else if ( g == h ){
      res = g;
    } else {
      NodeReference &rF = refs[f];
      NodeReference &rG = refs[g];
      NodeReference &rH = refs[h];

      Node const& F = nodes[rF.node_idx];
      Node const& G = nodes[rG.node_idx];
      Node const& H = nodes[rH.node_idx];

      var_t x;
      value_t f0, f1, g0, g1, h0, h1;
      if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
      {
        x = F.v;
        f0 = rF.flipped ? getFlippedReference(F.E) : F.E;
        f1 = rF.flipped ? getFlippedReference(F.T) : F.T;
        if ( G.v == F.v )
        {
          g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
          g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
        }
        else
        {
          g0 = g1 = g;
        }
        if ( H.v == F.v )
        {
          h0 = rH.flipped ? getFlippedReference(H.E) : H.E;
          h1 = rH.flipped ? getFlippedReference(H.T) : H.T;
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
          g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
          g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
          h0 = h1 = h;
        }
        else if ( H.v < G.v )
        {
          x = H.v;
          g0 = g1 = g;
          h0 = rH.flipped ? getFlippedReference(H.E) : H.E;
          h1 = rH.flipped ? getFlippedReference(H.T) : H.T;
        }
        else /* G.v == H.v */
        {
          x = G.v;
          g0 = rG.flipped ? getFlippedReference(G.E) : G.E;
          g1 = rG.flipped ? getFlippedReference(G.T) : G.T;
          h0 = rH.flipped ? getFlippedReference(H.E) : H.E;
          h1 = rH.flipped ? getFlippedReference(H.T) : H.T;
        }
      }

      value_t r0, r1;

      auto r0_index = std::make_tuple(f0, g0, h0);
      auto r0_search = ite_uni.find(r0_index);
      if(r0_search != ite_uni.end()){
        r0 = r0_search->second;
      } else {
        r0 = ITE( f0, g0, h0 );
      }

      auto r1_index = std::make_tuple(f1, g1, h1);
      auto r1_search = ite_uni.find(r1_index);
      if(r1_search != ite_uni.end()){
        r1 = r1_search->second;
      } else {
        r1 = ITE(f1, g1, h1);
      }

      res = unique(x, r1, r0);
    }

    ite_uni[std::make_tuple(f, g, h)] = res;
    ite_uni[std::make_tuple(f, getFlippedReference(g), getFlippedReference(h))] = getFlippedReference(res);
    ite_uni[std::make_tuple(getFlippedReference(f), h, g)] = res;
    ite_uni[std::make_tuple(getFlippedReference(f), getFlippedReference(h), getFlippedReference(g))] = getFlippedReference(res);

    return res;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( value_t f, std::ostream& os = std::cout ) const
  {
    for ( auto i = 0u; i < nodes[f].v; ++i )
    {
      os << "  ";
    }
    if ( f <= 1 )
    {
      os << "node " << f << ": constant " << f << std::endl;
    }
    else
    {
      os << "node " << f << ": var = " << nodes[f].v << ", T = " << nodes[f].T 
         << ", E = " << nodes[f].E << std::endl;
      for ( auto i = 0u; i < nodes[f].v; ++i )
      {
        os << "  ";
      }
      os << "> THEN branch" << std::endl;
      print( nodes[f].T, os );
      for ( auto i = 0u; i < nodes[f].v; ++i )
      {
        os << "  ";
      }
      os << "> ELSE branch" << std::endl;
      print( nodes[f].E, os );
    }
  }

  /* Get the truth table of the BDD rooted at node f. */
  Truth_Table get_tt( value_t f ) const
  {
    assert( f < refs.size() && "Make sure f exists." );
    //assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );

    if ( f == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }

    NodeReference rF = refs[f];
    Node nF = nodes[rF.node_idx];
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    var_t const x = nF.v;
    index_t const fx = nF.T;
    index_t const fnx = nF.E;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    auto res = ( tt_x & get_tt( fx ) ) | ( tt_nx & get_tt( fnx ) );
    if(rF.flipped)
      return ~res;
    return res;
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 1u; i < nodes.size(); ++i )
    {
      if ( nodes[i].reference_count != 0)
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( value_t f ) const
  {
    assert(f < refs.size() && "Make sure f exists.");
    //assert( f < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) || f == constant( true ) )
    {
      return 0u;
    }



    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    visited[1] = true;

    return num_nodes_rec( refs[f].node_idx, visited );
  }

  uint64_t num_invoke() const
  {
    return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite - repeat_invoke;
  }

private:
  /**********************************************************/
  /******************** Helper Functions ********************/
  /**********************************************************/

  uint64_t num_nodes_rec( index_t f, std::vector<bool>& visited ) const
  {
    assert( f < nodes.size() && "Make sure f exists." );
    

    uint64_t n = 0u;
    Node const& F = nodes[f];
    assert( F.T < refs.size() && "Make sure the children exist." );
    assert( F.E < refs.size() && "Make sure the children exist." );
    index_t tIndex = refs[F.T].node_idx;
    assert(tIndex < nodes.size() && "Make sure the children point to a valid node.");
    if ( !visited[tIndex] )
    {
      n += num_nodes_rec( tIndex, visited );
      visited[tIndex] = true;
    }
    index_t eIndex = refs[F.E].node_idx;
    assert(eIndex < nodes.size() && "Make sure the children point to a valid node.");
    if ( !visited[eIndex] )
    {
      n += num_nodes_rec( eIndex, visited );
      visited[eIndex] = true;
    }
    return n + 1u;
  }

  value_t getFlippedReference(value_t ref){
    NodeReference &n = refs[ref];
    return buildReference(!n.flipped, n.node_idx);
  }

  value_t buildReference(bool flipped, index_t node_idx){
    Node &node = nodes[node_idx];
    if(flipped && node.neg_ref != std::numeric_limits<index_t>::max())
      return node.neg_ref;
    else if(!flipped && node.pos_ref != std::numeric_limits<index_t>::max())
      return node.pos_ref;
    else {
      auto res = refs.size(); // get the size now.
      refs.emplace_back(NodeReference{
        flipped, node_idx
      });
      if(flipped)
        node.neg_ref = res;
      else
        node.pos_ref = res;
      return res;
    }
  }

private:
  std::vector<Node> nodes;
  std::vector<NodeReference> refs;
  std::vector<std::unordered_map<std::pair<value_t, value_t>, index_t>> unique_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */

  using binary_unique_table_t = std::unordered_map<std::pair<value_t, value_t>, value_t>;
  using trinary_unique_table_t = std::unordered_map<std::tuple<value_t, value_t, value_t>, value_t>;

  binary_unique_table_t and_uni;
  binary_unique_table_t or_uni;
  binary_unique_table_t xor_uni;

  std::pair<bool, value_t> lookUpBinaryComputedTable(const binary_unique_table_t &table, value_t f, value_t g) const {
    if(f > g) std::swap(f, g);
    auto index = std::make_pair(f, g);
    auto res = table.find(index);
    if(res != table.end()){
      return std::make_pair(true, res->second);
    }
    return std::make_pair(false, 0); // prefer std::optional? We can not use C++17 right?
  }

  void registerToBinaryComputedTable(binary_unique_table_t &t, value_t f, value_t g, value_t res){
    if(f > g) std::swap(f, g);
    auto index = std::make_pair(f, g);
    t[index] = res;
  }

  trinary_unique_table_t ite_uni;

  /* statistics */
  uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
  uint64_t repeat_invoke;
};
