#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>

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

private:
  struct Node
  {
    var_t v; /* corresponding variable */
    index_t T; /* index of THEN child */
    index_t E; /* index of ELSE child */
    bool T_comp; /* if THEN edge is complemented */
    bool E_comp; /* if ELSE edge is complemented */
    bool f_comp; /* if f -> x0 edge is complemented */
  };

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_not( 0u ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back( Node({num_vars, 0, 0, false, false, false}) ); /* constant 0 */
    nodes.emplace_back( Node({num_vars, 1, 1, false, false, false}) ); /* constant 1 */
    nodes_copy.emplace_back(Node({ num_vars, 0, 0, false, false, false })); /* constant 0 */
    nodes_copy.emplace_back(Node({ num_vars, 1, 1, false, false, false })); /* constant 1 */
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

  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  index_t unique( var_t var, index_t T, index_t E )
  {
      //std::cout << "BDD UNIQUE" << std::endl;
      //for (auto i = 0u; i < nodes.size(); ++i) print_one(nodes[i]);

      //std::cout << "var " << var << " T " << T << " E " << E << std::endl;
      //std::cout << "T.v " << nodes[T].v << std::endl;

    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( T < nodes.size() && "Make sure the children exist." );
    assert( E < nodes.size() && "Make sure the children exist." );
    assert( nodes[T].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[E].v > var && "With static variable order, children can only be below the node." );

    /* Reduction rule: Identical children */
    if ( (T == E) )
    {
      return T;
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find({ T, E });
    if (it != unique_table[var].end())
    {
      /* the required node already exists. return it. */
      return it->second;
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      index_t const new_index = nodes.size();
      nodes.emplace_back(Node({ var, T, E, false, false, false }));
      nodes_copy = nodes;
      unique_table[var][{T, E}] = new_index;
      
      /* complement edges when the BDD structure is completed */
      /*if ((var == 0) && (new_index != 2))
      {
          std::cout << "une\n";
          complement_bdd(new_index);
          /* nodes size has changed */
          /*return nodes.size() - 1;
      }*/

      return new_index;
    }
  }

  /* Rearrange BDD to make it cleaner. */
  void rearrange_bdd(index_t f)
  {
      std::vector<Node> nodes_temp;
      std::vector<Node> nodes_unchanged;
      std::vector<index_t> keep_index;

      nodes_unchanged = nodes;
      
      /* find used nodes */
      for (auto i = f; i > 0u; --i)
      {
          /* necessary nodes */
          if ((i == f) || (i == 2) || (nodes[i].v == num_vars()))
          {
              keep_index.push_back(i);
          }

          const auto it = std::find(keep_index.begin(), keep_index.end(), i);
          if (it != keep_index.end())
          {
              keep_index.push_back(i);
              keep_index.push_back(nodes[i].T);
              keep_index.push_back(nodes[i].E);
          }
      }
      sort(keep_index.begin(), keep_index.end());

      /* new nodes vector */
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          const auto it = std::find(keep_index.begin(), keep_index.end(), i);
          if (it != keep_index.end())
          {
              nodes_temp.push_back(nodes[i]);
          }
      }

      /* sort nodes by variable */
      sort(nodes_temp.begin()+3, nodes_temp.end(),
          [](const Node& n, const Node& m)
          { return (n.v > m.v); });
      nodes = nodes_temp;
      shift_indices(nodes_unchanged);
  }

  /* Add complemented edges. */
  void complement_bdd(index_t f) {
      /*std::cout << "BDD DEBUT" << std::endl;
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          std::cout << "i: " << i << " ";
          print_one(nodes[i]);
      }
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      std::vector<std::pair<index_t, bool>> previous_nodes;
      bool then_comp = false;

      /* trivial cases */
      if (f == 0)
      {
          nodes.clear();
          nodes.emplace_back(Node({ num_vars(), 1, 1, false, false, true }));
          return;
      }
      if (f == 1)
      {
          nodes.clear();
          nodes.emplace_back(Node({ num_vars(), 1, 1, false, false, false }));
          return;
      }

      /* complement edges to terminal nodes */
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          if ((nodes[i].v != num_vars()) && (unique_table[nodes[i].v][{nodes[i].T, nodes[i].E}] != 2))
          {
              if (nodes[i].E == 0)
              {
                  nodes[i].E_comp = true;
              }
              if (nodes[i].T == 0)
              {
                  nodes[i].T_comp = true;
                  then_comp = true;
              }
          }
      }

      /*std::cout << "BDD COMP SIMPLE" << std::endl;
      for (int i = 0; i < nodes.size(); ++i)
      {
          std::cout << "i: " << i << " ";
          print_one(nodes[i]);
      }
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      /* remove complements from THEN edges */
      while (then_comp)
      {
          for (auto i = f; i > 0u; --i)
          {
              /*std::cout << "NODE EN QUESTION\n";
              print_one(nodes[i]);
              std::cout << "BDD BOUCLE" << std::endl;
              for (int i = 0; i < nodes.size(); ++i)
              {
                  std::cout << "i: " << i << " ";
                  print_one(nodes[i]);
              }
              std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

              /* edge f -> x0 */
              if ((i == f) && (nodes[i].T_comp == true))
              {
                  nodes[i].T_comp = !nodes[i].T_comp;
                  nodes[i].E_comp = !nodes[i].E_comp;
                  nodes[i].f_comp = !nodes[i].f_comp;
              }

              /* complement equivalence */
              if (nodes[i].v != num_vars())
              {
                  previous_nodes = get_previous_nodes(i);
                  if (nodes[i].T_comp == true)
                  {
                      nodes[i].T_comp = !nodes[i].T_comp;
                      nodes[i].E_comp = !nodes[i].E_comp;
                      for (auto j = 0u; j < previous_nodes.size(); ++j)
                      {
                          index_t prev = std::get<0>(previous_nodes[j]);
                          //std::cout << "nodes i: " << i << " has previous: " << prev << std::endl;
                          if (std::get<1>(previous_nodes[j]) == true)
                              nodes[prev].T_comp = !nodes[prev].T_comp;
                          else
                              nodes[prev].E_comp = !nodes[prev].E_comp;
                      }
                  }
              }
          }
          then_comp = then_complemented();
      }

      /*std::cout << "BDD APRES" << std::endl;
      for (int i = 0; i < nodes.size(); ++i)
      {
          std::cout << "i: " << i << " ";
          print_one(nodes[i]);
      }
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      update_nodes(f);
      /*std::cout << "BDD NODE COOOOOOL" << std::endl;
      for (int i = 0; i < nodes.size(); ++i) print_one(nodes[i]);
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/
  }

  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  index_t literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ) );
  }

  /**********************************************************/
  /*********************** Ref & Deref **********************/
  /**********************************************************/

  index_t ref(index_t f)
  {
      /* TODO */
      return f;
  }

  void deref(index_t f)
  {
      /* TODO */
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
  index_t NOT( index_t f )
  {
    assert( f < nodes.size() && "Make sure f exists." );

    //++num_invoke_not;

    /* inspect hash table */
    auto t = std::make_tuple(f, f, f, "NOT");
    auto result = std::make_tuple(false, f);
    result = inspect_table(t);

    /* operation already computed */
    if (std::get<0>(result))
        return hash_result[std::get<1>(result)];

    /* trivial cases */
    if ( f == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, f, f, "NOT"));
      hash_result.push_back(constant(true));
      return constant( true );
    }
    if ( f == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, f, f, "NOT"));
      hash_result.push_back(constant(false));
      return constant( false );
    }

    Node const& F = nodes[f];
    var_t x = F.v;
    index_t f0 = F.E, f1 = F.T;
    
    index_t const r0 = NOT( f0 );
    index_t const r1 = NOT( f1 );

    hash_table.push_back(std::make_tuple(f, f, f, "NOT"));
    index_t const res = unique(x, r1, r0);
    hash_result.push_back(res);
    return res;
  }

  /* Compute f ^ g */
  index_t XOR( index_t f, index_t g )
  {
    assert( f < nodes.size() && "Make sure f exists." );
    assert( g < nodes.size() && "Make sure g exists." );

    ++num_invoke_xor;

    /* inspect hash table */
    auto t1 = std::make_tuple(f, g, f, "XOR");
    auto t2 = std::make_tuple(g, f, g, "XOR");
    auto result1 = std::make_tuple(false, f);
    auto result2 = std::make_tuple(false, g);
    result1 = inspect_table(t1);
    result2 = inspect_table(t2);

    /* operation already computed */
    if (std::get<0>(result1))
        return hash_result[std::get<1>(result1)];
    if (std::get<0>(result2))
        return hash_result[std::get<1>(result2)];

    /* trivial cases */
    if ( f == g )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(constant(false));
      hash_result.push_back(constant(false));
      return constant( false );
    }
    if ( f == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(g);
      hash_result.push_back(g);
      return g;
    }
    if ( g == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(f);
      hash_result.push_back(f);
      return f;
    }
    if ( f == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(NOT(g));
      hash_result.push_back(NOT(g));
      return NOT( g );
    }
    if ( g == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(NOT(f));
      hash_result.push_back(NOT(f));
      return NOT( f );
    }
    if ( f == NOT( g ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
      hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
      hash_result.push_back(constant(true));
      hash_result.push_back(constant(true));
      return constant( true );
    }

    Node const& F = nodes[f];
    Node const& G = nodes[g];
    var_t x;
    index_t f0, f1, g0, g1;
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

    index_t const r0 = XOR( f0, g0 );
    index_t const r1 = XOR( f1, g1 );

    hash_table.push_back(std::make_tuple(f, g, f, "XOR"));
    hash_table.push_back(std::make_tuple(g, f, g, "XOR"));
    index_t const res = unique(x, r1, r0);
    hash_result.push_back(res);
    hash_result.push_back(res);
    return res;
  }

  /* Compute f & g */
  index_t AND( index_t f, index_t g )
  {
    assert( f < nodes.size() && "Make sure f exists." );
    assert( g < nodes.size() && "Make sure g exists." );

    ++num_invoke_and;

    /* inspect hash table */
    auto t1 = std::make_tuple(f, g, f, "AND");
    auto t2 = std::make_tuple(g, f, g, "AND");
    auto result1 = std::make_tuple(false, f);
    auto result2 = std::make_tuple(false, g);
    result1 = inspect_table(t1);
    result2 = inspect_table(t2);
    
    /* operation already computed */
    if (std::get<0>(result1))
        return hash_result[std::get<1>(result1)];
    if (std::get<0>(result2))
        return hash_result[std::get<1>(result2)];

    /* trivial cases */
    if ( f == constant( false ) || g == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "AND"));
      hash_table.push_back(std::make_tuple(g, f, g, "AND"));
      hash_result.push_back(constant(false));
      hash_result.push_back(constant(false));
      return constant( false );
    }
    if ( f == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "AND"));
      hash_table.push_back(std::make_tuple(g, f, g, "AND"));
      hash_result.push_back(g);
      hash_result.push_back(g);
      return g;
    }
    if ( g == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "AND"));
      hash_table.push_back(std::make_tuple(g, f, g, "AND"));
      hash_result.push_back(f);
      hash_result.push_back(f);
      return f;
    }
    if ( f == g )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "AND"));
      hash_table.push_back(std::make_tuple(g, f, g, "AND"));
      hash_result.push_back(f);
      hash_result.push_back(f);
      return f;
    }

    Node const& F = nodes[f];
    Node const& G = nodes[g];
    var_t x;
    index_t f0, f1, g0, g1;
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

    index_t const r0 = AND( f0, g0 );
    index_t const r1 = AND( f1, g1 );

    hash_table.push_back(std::make_tuple(f, g, f, "AND"));
    hash_table.push_back(std::make_tuple(g, f, g, "AND"));
    index_t const res = unique(x, r1, r0);
    hash_result.push_back(res);
    hash_result.push_back(res);
    return res;
  }

  /* Compute f | g */
  index_t OR( index_t f, index_t g )
  {
    assert( f < nodes.size() && "Make sure f exists." );
    assert( g < nodes.size() && "Make sure g exists." );

    ++num_invoke_or;

    /* inspect hash table */
    auto t1 = std::make_tuple(f, g, f, "OR");
    auto t2 = std::make_tuple(g, f, g, "OR");
    auto result1 = std::make_tuple(false, f);
    auto result2 = std::make_tuple(false, g);
    result1 = inspect_table(t1);
    result2 = inspect_table(t2);

    /* operation already computed */
    if (std::get<0>(result1))
        return hash_result[std::get<1>(result1)];
    if (std::get<0>(result2))
        return hash_result[std::get<1>(result2)];
    
    /* trivial cases */
    if ( f == constant( true ) || g == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "OR"));
      hash_table.push_back(std::make_tuple(g, f, g, "OR"));
      hash_result.push_back(constant(true));
      hash_result.push_back(constant(true));
      return constant( true );
    }
    if ( f == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "OR"));
      hash_table.push_back(std::make_tuple(g, f, g, "OR"));
      hash_result.push_back(g);
      hash_result.push_back(g);
      return g;
    }
    if ( g == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "OR"));
      hash_table.push_back(std::make_tuple(g, f, g, "OR"));
      hash_result.push_back(f);
      hash_result.push_back(f);
      return f;
    }
    if ( f == g )
    {
      hash_table.push_back(std::make_tuple(f, g, f, "OR"));
      hash_table.push_back(std::make_tuple(g, f, g, "OR"));
      hash_result.push_back(f);
      hash_result.push_back(f);
      return f;
    }

    Node const& F = nodes[f];
    Node const& G = nodes[g];
    var_t x;
    index_t f0, f1, g0, g1;
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

    index_t const r0 = OR( f0, g0 );
    index_t const r1 = OR( f1, g1 );

    hash_table.push_back(std::make_tuple(f, g, f, "OR"));
    hash_table.push_back(std::make_tuple(g, f, g, "OR"));
    index_t const res = unique(x, r1, r0);
    hash_result.push_back(res);
    hash_result.push_back(res);
    return res;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  index_t ITE( index_t f, index_t g, index_t h )
  {
    assert( f < nodes.size() && "Make sure f exists." );
    assert( g < nodes.size() && "Make sure g exists." );
    assert( h < nodes.size() && "Make sure h exists." );

    ++num_invoke_ite;

    /* inspect hash table */
    auto t1 = std::make_tuple(f, g, h, "ITE");
    auto t2 = std::make_tuple(NOT(f), h, g, "ITE");
    auto result1 = std::make_tuple(false, f);
    auto result2 = std::make_tuple(false, NOT(f));
    result1 = inspect_table(t1);
    result2 = inspect_table(t2);

    /* operation already computed */
    if (std::get<0>(result1))
        return hash_result[std::get<1>(result1)];
    if (std::get<0>(result2))
        return hash_result[std::get<1>(result2)];

    /* trivial cases */
    if ( f == constant( true ) )
    {
      hash_table.push_back(std::make_tuple(f, g, h, "ITE"));
      hash_table.push_back(std::make_tuple(NOT(f), h, g, "ITE"));
      hash_result.push_back(g);
      hash_result.push_back(g);
      return g;
    }
    if ( f == constant( false ) )
    {
      hash_table.push_back(std::make_tuple(f, g, h, "ITE"));
      hash_table.push_back(std::make_tuple(NOT(f), h, g, "ITE"));
      hash_result.push_back(h);
      hash_result.push_back(h);
      return h;
    }
    if ( g == h )
    {
      hash_table.push_back(std::make_tuple(f, g, h, "ITE"));
      hash_table.push_back(std::make_tuple(NOT(f), h, g, "ITE"));
      hash_result.push_back(g);
      hash_result.push_back(g);
      return g;
    }

    Node const& F = nodes[f];
    Node const& G = nodes[g];
    Node const& H = nodes[h];
    var_t x;
    index_t f0, f1, g0, g1, h0, h1;
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

    index_t const r0 = ITE( f0, g0, h0 );
    index_t const r1 = ITE( f1, g1, h1 );

    hash_table.push_back(std::make_tuple(f, g, h, "ITE"));
    hash_table.push_back(std::make_tuple(NOT(f), h, g, "ITE"));
    index_t const res = unique(x, r1, r0);
    hash_result.push_back(res);
    hash_result.push_back(res);
    return res;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( index_t f, std::ostream& os = std::cout ) const
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

  Truth_Table get_tt( index_t f, uint8_t self_call = 0u ) //const
  {
    if (!self_call)
        nodes = nodes_copy;

    assert( f < nodes.size() && "Make sure f exists." );
    //assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );

    if (!self_call)
    {
        first_call++;
        if ((f != 0) && (f != 1))
        {
            rearrange_bdd(f);
            f = nodes.size() - 1;
        }
        /*std::cout << "BDD REARRANGED\n";
        for (int i = 0; i < nodes.size(); ++i)
        {
            std::cout << "i: " << i << " ";
            print_one(nodes[i]);
        }*/
        complement_bdd(f);
        f = nodes.size() - 1;
        /*std::cout << "BDD compppp\n";
        for (int i = 0; i < nodes.size(); ++i)
        {
            std::cout << "i: " << i << " ";
            print_one(nodes[i]);
        }
        std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/
    }

    if ( f == constant( false ) )
    {
      /* constant false (node 0) actually represents constant true (due to complemented bdd) */
      return !nodes[f].f_comp ? ~Truth_Table( num_vars() ) : Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      /* should never be called */
      return Truth_Table( num_vars() );
    }
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    var_t const x = nodes[f].v;
    index_t const fx = nodes[f].T;
    index_t const fnx = nodes[f].E;
    bool const e_comp = nodes[f].E_comp;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    
    /* if edge f -> xi is complemented */
    if (nodes[f].f_comp)
    {
        return operator~((tt_x & get_tt(fx, true)) | (!e_comp ? (tt_nx & get_tt(fnx, true)) : (tt_nx & operator~(get_tt(fnx, true)))));
    }

    return (tt_x & get_tt(fx, true)) | (!e_comp ? (tt_nx & get_tt(fnx, true)) : (tt_nx & operator~(get_tt(fnx, true))));
  }

  /* Whether `f` is dead (having a reference count of 0). */
  bool is_dead( index_t f ) const
  {
    /* NOT DONE SINCE GARBAGE COLLECTION WAS IMPLEMENTED THROUGH COMPLEMENTED EDGES BDD ALGORITHM */
    return false;
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() //const
  {
    /* assign the real nodes if multiple calls to get_tt() */
      if (first_call > 1)
      {
          nodes = nodes_copy;
          index_t f = nodes.size() - 1;
          if ((f != 0) && (f != 1))
          {
              rearrange_bdd_for_count(f);
              f = nodes.size() - 1;
          }
          /*std::cout << "BDD AVANT COMP\n";
          for (int i = 0; i < nodes.size(); ++i)
          {
              std::cout << "i: " << i << " ";
              print_one(nodes[i]);
          }
          complement_bdd(f);
          
          std::cout << "BDD NUM\n";
          for (int i = 0; i < nodes.size(); ++i)
          {
              std::cout << "i: " << i << " ";
              print_one(nodes[i]);
          }*/
      }

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
  uint64_t num_nodes( index_t f ) const
  {
    /* reassign f after complementing the BDD edges */
    f = nodes.size() - 1;

    assert( f < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) || f == constant( true ) )
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
    return /*num_invoke_not +*/ num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
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
    assert( F.T < nodes.size() && "Make sure the children exist." );
    assert( F.E < nodes.size() && "Make sure the children exist." );
    if ( !visited[F.T] )
    {
      n += num_nodes_rec( F.T, visited );
      visited[F.T] = true;
    }
    if ( !visited[F.E] )
    {
      n += num_nodes_rec( F.E, visited );
      visited[F.E] = true;
    }
    return n + 1u;
  }

  /* Inspect the hash table */
  std::tuple<bool, index_t> inspect_table(std::tuple<index_t, index_t, index_t, std::string> t)
  {
    bool found = false;
    auto it = std::find_if(hash_table.begin(), hash_table.end(), [&t](const auto& item) {
        return std::get<0>(t) == std::get<0>(item)
            && std::get<1>(t) == std::get<1>(item)
            && std::get<2>(t) == std::get<2>(item)
            && std::get<3>(t) == std::get<3>(item);
        });

    if (it != hash_table.end())
    {
        found = true;
    }
    uint32_t index = std::distance(hash_table.begin(), it);
    
    auto res = std::make_tuple(found, index);
    return res;
  }

  /* Return list of previous nodes. */
  std::vector<std::pair<index_t, bool>> get_previous_nodes(index_t f)
  {
      std::vector<std::pair<index_t, bool>> list_previous;
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          if (nodes[i].T == f)
              list_previous.push_back(std::make_pair((index_t)i, true)); /* true = THEN edge */
          if (nodes[i].E == f)
              list_previous.push_back(std::make_pair((index_t)i, false)); /* false = ELSE edge */
      }
      return list_previous;
  }

  /* Removing and merging nodes. */
  void update_nodes(index_t f)
  {
      std::vector<std::vector<index_t>> list_equiv;
      std::vector<index_t> equiv;
      std::vector<Node> nodes_temp;
      std::vector<Node> nodes_unchanged;
      std::vector<index_t> to_remove;
      var_t var;
      index_t T;
      index_t E;
      index_t E_table;
      bool T_comp;
      bool E_comp;

      /* keep a copy of nodes and update it when needed */
      nodes_unchanged = nodes;

      /* removing edges to terminal node 0 */
      remove_terminal_0();

      /*std::cout << "BDD REMOVE 0" << std::endl;
      for (int i = 0; i < nodes.size(); ++i) print_one(nodes[i]);
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      /* find equivalent nodes */
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          var = nodes[i].v;
          T = nodes[i].T;
          E = nodes[i].E;
          T_comp = nodes[i].T_comp;
          E_comp = nodes[i].E_comp;
          E_table = (E == 1) ? (E - nodes[i].E_comp) : E;
          equiv.push_back(i);

          for (auto j = 0u; j < nodes.size(); ++j)
          {
              if ((i != j) && (var == nodes[j].v) && (T == nodes[j].T) && (E == nodes[j].E)
                  && (T_comp == nodes[j].T_comp) && (E_comp == nodes[j].E_comp))
              {
                  equiv.push_back(j);
              }
          }
          sort(equiv.begin(), equiv.end());
          /* only add the vector if it wasn't already computed */
          const auto it = std::find(list_equiv.begin(), list_equiv.end(), equiv);
          if (it == list_equiv.end())
              list_equiv.push_back(equiv);
          equiv.clear();
      }

      /* update indices and get indices to delete */
      for (auto i = 0u; i < list_equiv.size(); ++i)
      {
          update_index(list_equiv[i]);
          to_remove = indices_to_delete(list_equiv[i], to_remove);
      }
      sort(to_remove.begin(), to_remove.end());
      nodes_unchanged = nodes;

      /*std::cout << "BDD UPDATE IND" << std::endl;
      for (int i = 0; i < nodes.size(); ++i) print_one(nodes[i]);
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      /* delete nodes */
      for (auto i = 1u; i < nodes.size(); ++i)
      {
          const auto it = std::find(to_remove.begin(), to_remove.end(), i);
          if (it == to_remove.end())
          {
              nodes_temp.push_back(nodes[i]);
          }
      }
      nodes = nodes_temp;

      /*std::cout << "BDD DELETE IND" << std::endl;
      for (int i = 0; i < nodes.size(); ++i) print_one(nodes[i]);
      std::cout << "f -> x0 comp? " << nodes[f].f_comp << std::endl;*/

      /* shift indices */
      shift_indices(nodes_unchanged);
  }

  /* Shift indices for the new BDD after deleting nodes. */
  void shift_indices(std::vector<Node> nodes_unchanged)
  {
      var_t var;
      index_t T;
      index_t E;
      uint32_t id;
      std::vector<index_t> old_index;
      std::vector<index_t> new_index;

      for (auto i = 0u; i < nodes_unchanged.size(); ++i)
      {
          for (auto j = 0u; j < nodes.size(); ++j)
          {
              if ((nodes_unchanged[i].v == nodes[j].v) && (nodes_unchanged[i].T == nodes[j].T)
                  && (nodes_unchanged[i].E == nodes[j].E) && (nodes_unchanged[i].T_comp == nodes[j].T_comp)
                  && (nodes_unchanged[i].E_comp == nodes[j].E_comp))
              {
                  old_index.push_back(i);
                  new_index.push_back(j);
              }
          }
      }

      for (auto i = 2u; i < nodes.size(); ++i)
      {
          const auto it_T = std::find(old_index.begin(), old_index.end(), nodes[i].T);
          if (it_T != old_index.end())
          {
              id = std::distance(old_index.begin(), it_T);
              nodes[i].T = new_index[id];
          }
          else
          {
              std::cout << "Problem in T, index should be found." << std::endl;
          }

          const auto it_E = std::find(old_index.begin(), old_index.end(), nodes[i].E);
          if (it_E != old_index.end())
          {
              id = std::distance(old_index.begin(), it_E);
              nodes[i].E = new_index[id];
          }
          else
          {
              std::cout << "Problem in E, index should be found." << std::endl;
          }
      }
  }

  /* Find indices of nodes equivalent to others nodes that could be deleted. */
  std::vector<index_t> indices_to_delete(std::vector<index_t> equiv, std::vector<index_t> to_remove)
  {
      var_t var;
      index_t T;
      index_t E;
      index_t E_table;

      for (auto j = 0u; j < nodes.size(); ++j)
      {
          var = nodes[j].v;
          T = nodes[j].T;
          E = nodes[j].E;
          E_table = (E == 1) ? (E - nodes[j].E_comp) : E;

          if (var != num_vars()) {
              const auto it = std::find(equiv.begin(), equiv.end(), j);
              if ((it != equiv.end()) && (j != equiv[0]))
              {
                  to_remove.push_back(unique_table[var][{T, E_table}]);
              }
          }
      }
      return to_remove;
  }

  /* Update indices for nodes having edges to equivalent nodes. */
  void update_index(std::vector<index_t> equiv)
  {
      if (equiv.size() > 1)
      {
          for (auto i = 0u; i < nodes.size(); ++i)
          {
              const auto it_T = std::find(equiv.begin(), equiv.end(), nodes[i].T);
              if (it_T != equiv.end())
              {
                  nodes[i].T = equiv[0];
              }

              const auto it_E = std::find(equiv.begin(), equiv.end(), nodes[i].E);
              if (it_E != equiv.end())
              {
                  nodes[i].E = equiv[0];
              }
          }
      }
  }

  /* Remove terminal node 0. */
  void remove_terminal_0()
  {
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          if ((nodes[i].v != num_vars()) && (unique_table[nodes[i].v][{nodes[i].T, nodes[i].E}] != 2))
          {
              if (nodes[i].E == 0)
              {
                  nodes[i].E = 1;
              }
              if (nodes[i].T == 0)
              {
                  nodes[i].T = 1;
              }
          }
      }
  }

  /* Check if a complemented THEN edge exists. */
  bool then_complemented()
  {
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          if (nodes[i].T_comp == true)
              return true;
      }

      return false;
  }

  /* Rearrange BDD to make it cleaner for count. */
  void rearrange_bdd_for_count(index_t f)
  {
      std::vector<Node> nodes_temp;
      std::vector<Node> nodes_unchanged;
      std::vector<index_t> keep_index;

      nodes_unchanged = nodes;

      /* find used nodes */
      for (auto i = 0u; i <= f; ++i)
      {
          if ((i == f) || (i == 2) || (nodes[i].v == num_vars()))
          {
              keep_index.push_back(i);
              keep_index.push_back(nodes[i].T);
              keep_index.push_back(nodes[i].E);
          }
          else
          {
              keep_index.push_back(nodes[i].T);
              keep_index.push_back(nodes[i].E);
          }
      }
      sort(keep_index.begin(), keep_index.end());

      /* new nodes vector */
      for (auto i = 0u; i < nodes.size(); ++i)
      {
          const auto it = std::find(keep_index.begin(), keep_index.end(), i);
          if (it != keep_index.end())
          {
              nodes_temp.push_back(nodes[i]);
          }
      }

      /* sort nodes by variable */
      sort(nodes_temp.begin() + 3, nodes_temp.end(),
          [](const Node& n, const Node& m)
          { return (n.v > m.v); });
      nodes = nodes_temp;
      shift_indices(nodes_unchanged);
  }

  /* Print one node. */
  void print_one(const Node& s) {
      std::cout << "var: " << s.v << "  T: " << s.T
          << "  E: " << s.E << "  T_comp?: " << s.T_comp
          << "  E_comp?: " << s.E_comp << '\n';
  }

private:
  std::vector<Node> nodes;
  std::vector<Node> nodes_copy;
  std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */

  /* statistics */
  uint64_t num_invoke_not, num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;

  /* hash tables */
  std::vector<std::tuple<index_t, index_t, index_t, std::string>> hash_table;
  std::vector<index_t> hash_result;

  /* number of first calls of truth table */
  uint8_t first_call = 0u;
  /* truth_table called multiple times */
  bool multiple_call = false;
};
