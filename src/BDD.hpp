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

template<>
struct hash<tuple<uint32_t, uint32_t>>
{
  using argument_type = tuple<uint32_t, uint32_t>;
  using result_type = size_t;
  result_type operator() ( argument_type const& in ) const
  {
    result_type seed = 0;
    hash_combine( seed, std::get<0>( in ) );
    hash_combine( seed, std::get<1>( in ) );
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
    hash_combine( seed, std::get<0>( in ) );
    hash_combine( seed, std::get<1>( in ) );
    hash_combine( seed, std::get<2>( in ) );
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
   
  using signal_t = uint32_t;

  using var_t = uint32_t;
  /* Similarly, declare `var_t` also as an alias for an unsigned integer.
   * This datatype will be used for representing variables. */

private:
  struct Node
  {
    var_t v; /* corresponding variable */
    signal_t T; /* index of THEN child */
    signal_t E; /* index of ELSE child */
    uint32_t ref_count ; /*node reference count*/                                                   
  };
  
    //create signal
  inline signal_t make_signal( index_t index, bool complement = false ) const
  {
    return complement ? ( index << 1 ) + 1 : index << 1;
  }

	//get signal index
  inline index_t get_index( signal_t signal ) const
  {
    assert( ( signal >> 1 ) < nodes.size() );
    return signal >> 1;
  }

	// get the nodes of a signal
  inline Node get_node( signal_t signal ) const
  {
    return nodes[get_index( signal )];
  }

	//determine if a signal is complemented or not 
  inline bool is_complemented( signal_t signal ) const
  {
    return signal & 0x1;
  }
  
   // give the complement of a signal 
  inline signal_t complement_signal(signal_t signal) 
  {
	  return (signal ^ 0x1) ;
  }

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back( Node({num_vars, 0, 0, 0}) );    
                                  
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
  signal_t constant( bool value ) const
  {
    return !value ? 1 : 0; 
  }

  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  
  signal_t unique( var_t var, signal_t T, signal_t E )
  {
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( (get_index(T)) < nodes.size() && "Make sure the children exist." );
    assert( (get_index(E)) < nodes.size() && "Make sure the children exist." );
    assert( get_node(T).v > var && "With static variable order, children can only be below the node." );
    assert( get_node(E).v > var && "With static variable order, children can only be below the node." );
	
	bool complemented = false;
	
		/*T edge should not be complemeted */
	if (is_complemented(T) != 0){
		complemented = true;
		E = complement_signal (E);
		T = complement_signal (T);
	}
	

    /* Reduction rule: Identical children */
    if ( T == E )
    {
      return (T | complemented) ;
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T, E} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */
      return (it->second | complemented) ;
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      signal_t const new_index = nodes.size();
      nodes.emplace_back( Node({var, T, E,0}) );                                                                           
      unique_table[var][{T, E}] = new_index << 1;
      return complemented ? make_signal(new_index, true ): make_signal(new_index, false ) ;
    }
  }

  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  signal_t literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ) );
  }

/**********************************************************/
  /*********************** Ref & Deref **********************/
  /**********************************************************/
  
  //ref function
  signal_t ref( signal_t f )                                                                                                  
  {
    nodes[get_index(f)].ref_count ++;
    if (get_index(f) != 0){
		ref (nodes[get_index(f)].E);
		ref (nodes[get_index(f)].T);
	}
    return f;
  }

  //deref function
  void deref( signal_t f )                                                                                                    
  {
	if (nodes[get_index(f)].ref_count != 0) {
    nodes[get_index(f)].ref_count -- ;}
    if (get_index(f) != 0){
		deref (nodes[get_index(f)].E);
		deref (nodes[get_index(f)].T);
	}
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

		/* Compute ~f */
  signal_t NOT( signal_t f )
  {
	return complement_signal(f) ;
  }

		/* Compute f ^ g */
  signal_t XOR( signal_t f, signal_t g )
  {
    assert( ( get_index(f) ) < nodes.size() && "Make sure f exists." );
    assert( ( get_index(g) ) < nodes.size() && "Make sure g exists." );
    ++num_invoke_xor;
	
	bool f_complemented = is_complemented(f);
	bool g_complemented = is_complemented(g);
	
	const auto it = computed_table_XOR.find( std::make_tuple(f ,g ));
    if ( it != computed_table_XOR.end() ){
	    return it-> second ;
	}
	
	const auto its = computed_table_XOR.find( std::make_tuple(g ,f ));
    if ( its != computed_table_XOR.end() ){
	    return its-> second ;
	}
	
    /* trivial cases */
    if ( f == g )
    {
      return constant( false );
    }
    if ( f == constant( false ) )
    {
      return g;
    }
    if ( g == constant( false ) )
    {
      return f;
    }
    if ( f == constant( true ) )
    {
      return NOT( g );
    }
    if ( g == constant( true ) )
    {
      return NOT( f );
    }
    if ( f == NOT( g ) )
    {
      return constant( true );
    }

    Node const& F = get_node(f);
    Node const& G = get_node(g);
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E) : F.E;
      f1 = f_complemented ? F.T & (~0x1) : F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_complemented ? complement_signal(G.E) : G.E;
      g1 = g_complemented ? G.T & (~0x1) : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E) : F.E;
      f1 = f_complemented ? F.T & (~0x1) : F.T;
      g0 = g_complemented ? complement_signal(G.E) : G.E;
      g1 = g_complemented ? G.T & (~0x1) : G.T;
    }
	
    signal_t const r0 = XOR( f0, g0 );
    signal_t const r1 = XOR( f1, g1 );
    signal_t res=unique( x, r1, r0 );
    computed_table_XOR.emplace(std::make_tuple(f,g) , res );
    return res;
  }

		/* Compute f & g */
  signal_t AND( signal_t f, signal_t g )
  {
    assert( ( get_index(f) ) < nodes.size() && "Make sure f exists." );
    assert( ( get_index(g) ) < nodes.size() && "Make sure g exists." );
    ++num_invoke_and;
    
    bool f_complemented = is_complemented(f);
	bool g_complemented = is_complemented(g);
	
	const auto it = computed_table_AND.find( std::make_tuple(f ,g ));
    if ( it != computed_table_AND.end() ){
	    return it-> second ;
	}
	
	const auto its = computed_table_AND.find( std::make_tuple(g ,f ));
    if ( its != computed_table_AND.end() ){
	    return its-> second ;
	}
	
    /* trivial cases */
    if ( f == constant( false ) || g == constant( false ) )
    {
      return constant( false );
    }
    if ( f == constant( true ) )
    {
      return g;
    }
    if ( g == constant( true ) )
    {
      return f;
    }
    if ( f == g )
    {
      return f;
    }

    Node const& F = get_node(f);
    Node const& G = get_node(g);
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E): F.E;
      f1 = f_complemented ? complement_signal(F.T): F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_complemented ? complement_signal(G.E) : G.E;
      g1 = g_complemented ? complement_signal(G.T) : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E) : F.E;
      f1 = f_complemented ? complement_signal(F.T) : F.T;
      g0 = g_complemented ? complement_signal(G.E): G.E;
      g1 = g_complemented ? complement_signal(G.T) : G.T;
    }

    signal_t const r0 = AND( f0, g0 );
    signal_t const r1 = AND( f1, g1 );
    signal_t res=unique( x, r1, r0 );
    computed_table_AND.emplace(std::make_tuple(f,g) , res );
    return res;
  }
  
  /* Compute f | g */
  signal_t OR( signal_t f, signal_t g )
  {
    assert( ( get_index(f) ) < nodes.size() && "Make sure f exists." );
    assert( ( get_index(g) ) < nodes.size() && "Make sure g exists." );
    ++num_invoke_or;
    
    bool f_complemented = is_complemented(f);
	bool g_complemented = is_complemented(g);
	
		
	const auto it = computed_table_OR.find( std::make_tuple(f ,g ));
    if ( it != computed_table_OR.end() ){
	    return it-> second ;
	}
	
	const auto its = computed_table_OR.find( std::make_tuple(g ,f ));
    if ( its != computed_table_OR.end() ){
	    return its-> second ;
	}

    /* trivial cases */
    if ( f == constant( true ) || g == constant( true ) )
    {
      return constant( true );
    }
    if ( f == constant( false ) )
    {
      return g;
    }
    if ( g == constant( false ) )
    {
      return f;
    }
    if ( f == g )
    {
      return f;
    }

    Node const& F = get_node(f);
    Node const& G = get_node(g);
    
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E): F.E;
      f1 = f_complemented ? complement_signal(F.T) : F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_complemented ? complement_signal(G.E) : G.E;
      g1 = g_complemented ?complement_signal(G.T) : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E) : F.E;
      f1 = f_complemented ? complement_signal(F.T) : F.T;
      g0 = g_complemented ? complement_signal(G.E): G.E;
      g1 = g_complemented ? complement_signal(G.T) : G.T;
    }

    signal_t const r0 = OR( f0, g0 );
    signal_t const r1 = OR( f1, g1 );
    signal_t res=unique( x, r1, r0 );
    computed_table_OR.emplace(std::make_tuple(f,g) , res );
    return res;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  signal_t ITE( signal_t f, signal_t g, signal_t h )
  {
     assert( ( get_index(f) ) < nodes.size() && "Make sure f exists." );
     assert( ( get_index(g) ) < nodes.size() && "Make sure g exists." );
     assert( ( get_index(h) ) < nodes.size() && "Make sure g exists." );
    ++num_invoke_ite;
    
    bool f_complemented = is_complemented(f);
	bool g_complemented = is_complemented(g);
	bool h_complemented = is_complemented(h);
	
		
	const auto it = computed_table_ITE.find( std::make_tuple(f ,g ,h ));
    if ( it != computed_table_ITE.end() ){
	    return it-> second ;
	}
	
	const auto its = computed_table_ITE.find( std::make_tuple(complement_signal(f),h ,g ));
    if ( its != computed_table_ITE.end() ){
	    return its-> second ;
	}

    /* trivial cases */
    if ( f == constant( true ) )
    {
      return g;
    }
    if ( f == constant( false ) )
    {
      return h;
    }
    if ( g == h )
    {
      return g;
    }

    Node const& F = get_node(f);
    Node const& G = get_node(g);
    Node const& H = get_node(h);
    var_t x;
    signal_t f0, f1, g0, g1, h0, h1;
    if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
    {
      x = F.v;
      f0 = f_complemented ? complement_signal(F.E) : F.E;
      f1 = f_complemented ? complement_signal(F.T) : F.T;
      if ( G.v == F.v )
      {
        g0 = g_complemented ? complement_signal(G.E) : G.E;
        g1 = g_complemented ? complement_signal(G.T) : G.T;
      }
      else
      {
        g0 = g1 = g;
      }
      if ( H.v == F.v )
      {
        h0 = h_complemented ? complement_signal(H.E) : H.E;
        h1 = h_complemented ? complement_signal(H.T) : H.T;
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
        g0 = g_complemented ? complement_signal(G.E) : G.E;
        g1 = g_complemented ? complement_signal(G.T) : G.T;
        h0 = h1 = h;
      }
      else if ( H.v < G.v )
      {
        x = H.v;
        g0 = g1 = g;
        h0 = h_complemented ? complement_signal(H.E) : H.E;
        h1 = h_complemented ? complement_signal(H.T) : H.T;
      }
      else /* G.v == H.v */
      {
        x = G.v;
        g0 = g_complemented ? complement_signal(G.E) : G.E;
        g1 = g_complemented ? complement_signal(G.T): G.T;
        h0 = h_complemented ? complement_signal(H.E): H.E;
        h1 = h_complemented ? complement_signal(H.T): H.T;
      }
    }

    signal_t const r0 = ITE( f0, g0, h0 );
    signal_t const r1 = ITE( f1, g1, h1 );
    signal_t res=unique( x, r1, r0 );
    computed_table_ITE.emplace(std::make_tuple(f,g,h) , res );
    return res;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( signal_t f, std::ostream& os = std::cout ) const 
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


  /*get truth table*/
  Truth_Table get_tt( signal_t f ) const
  {
    assert( (get_index(f)) < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    bool node_complemented = is_complemented(f);
    bool else_complemented = is_complemented(get_node(f).E);
    
    var_t const x = get_node(f).v;
    signal_t const fx = get_node(f).T;
    signal_t const fnx = get_node(f).E;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
  
    if (not node_complemented and not else_complemented)
		return ( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & get_tt( fnx & (~0x1) ) );
		
	if (not node_complemented and  else_complemented)
		return ( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & ~get_tt( fnx & (~0x1) ) );
		
	if (node_complemented and  else_complemented)
		return ~(( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & ~get_tt( fnx & (~0x1) ) ));
		
	if ( node_complemented and not else_complemented)
		return ~(( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & get_tt( fnx & (~0x1) ) ));
		
  }

  /* Whether `f` is dead (having a reference count of 0). */
  bool is_dead( index_t f ) const                                                                                          
  {
    if (nodes[f].ref_count == 0 ) 
    {
		return true;
	}
	return false;
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 1u; i < nodes.size(); ++i )                                                                     
    {
      if ( !is_dead( i ) )
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( signal_t f ) const
  {
    assert( get_index(f) < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) || f == constant( true ) )
    {
      return 0u;
    }
    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    return num_nodes_rec( f, visited );
  }

  uint64_t num_invoke() const
  {
    return  num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
  }

private:
  /**********************************************************/
  /******************** Helper Functions ********************/
  /**********************************************************/

  uint64_t num_nodes_rec( signal_t f, std::vector<bool>& visited ) const
  {
    assert( (f>>1) < nodes.size() && "Make sure f exists." );
    
    uint64_t n = 0u;
    Node const& F = nodes[f>>1];
    assert( ( (F.T>>1) >> 1 ) < nodes.size() && "Make sure the children exist." );
    assert( ( (F.E>>1) >> 1 ) < nodes.size() && "Make sure the children exist." );
    if ( !visited[F.T>>1] )
    {
      n += num_nodes_rec( F.T, visited );
      visited[F.T>>1] = true;
    }
    if ( !visited[F.E>>1] )
    {
      n += num_nodes_rec( F.E, visited );
      visited[F.E>>1] = true;
    }
    return n + 1u;
  }

private:
  std::vector<Node> nodes;
  std::vector<std::unordered_map<std::pair<signal_t, signal_t>, signal_t>> unique_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */

  /* statistics */
  uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
  
  std::unordered_map<std::tuple<signal_t, signal_t>, signal_t> computed_table_AND;
  std::unordered_map<std::tuple<signal_t, signal_t>, signal_t> computed_table_OR;
  std::unordered_map<std::tuple<signal_t, signal_t>, signal_t> computed_table_XOR;
  std::unordered_map<std::tuple<signal_t, signal_t, signal_t>, signal_t> computed_table_ITE;
  

};
