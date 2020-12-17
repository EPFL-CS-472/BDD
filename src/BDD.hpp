#pragma once

#include "truth_table.hpp"
//test
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
/*-------------------------------------------------------------------------------ADDED HASH---------------------------------------------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
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
    uint32_t ref_count ; /*reference count*/            /*added ref_count to the structure  */
  };
  
  inline signal_t make_signal( index_t index, bool complement = false ) const
  {
    return complement ? ( index << 1 ) + 1 : index << 1;
  }

  inline index_t get_index( signal_t signal ) const
  {
    assert( ( signal >> 1 ) < nodes.size() );
    return signal >> 1;
  }

  inline Node get_node( signal_t signal ) const
  {
    return nodes[get_index( signal )];
  }

  inline bool is_complemented( signal_t signal ) const
  {
    return signal & 0x1;
  }
 

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back( Node({num_vars, 0, 0, 0}) ); /* constant 1 */ //the only node needed for Complemented BDD       
                                  
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
    return !value ? 1 : 0; // TRUE : return 1 complemented , FALSE  returns 0 non complemented
  }

  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  signal_t unique( var_t var, signal_t T, signal_t E )
  {
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( (get_index(T)) < nodes.size() && "Make sure the children exist." );
    assert( (get_index(E)) < nodes.size() && "Make sure the children exist." );
    assert( nodes[get_index(T)].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[get_index(E)].v > var && "With static variable order, children can only be below the node." );
	
	bool node_is_complemented = false;
	
	if (is_complemented( T ) != 0){   /* verify if the THEN edge is complemeted , we complement the ELSE */
		node_is_complemented = true;
		
	
		E=E ^ 0x1;
		T=T ^ 0x1;
	}
	

    /* Reduction rule: Identical children */
    if ( T == E )
    {
      return T | node_is_complemented ;
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T, E} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */
      return (it->second | node_is_complemented) ;
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      signal_t const new_index = nodes.size();
      //std::cout << "nodes.size() : "  <<  nodes.size() << std::endl;
      //nodes.emplace_back( Node({var, T, E,0}) ); 
      nodes.emplace_back( Node({var, T, E,0}) );                                                                                
      
      
      
      
      unique_table[var][{T, E}] = new_index << 1;      /* new_index << 1 is the shift in order to add a non complemented signal to the unique table */
      //std::cout << "returned index : "  <<  (node_is_complemented ? (new_index << 1) + 1 : (new_index << 1)) << std::endl;
      return node_is_complemented ? (new_index << 1) + 1 : (new_index << 1) ;
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
  signal_t ref( signal_t f )                  /*for ref and deref, we go through the nodes using index which corresponds to one right shift of the signal_t*/                                                                                 
  {
    nodes[get_index(f)].ref_count ++;
    if (get_index(f) != 0){
		ref (nodes[get_index(f)].E);
		ref (nodes[get_index(f)].T);
	}
    
    return f;
  }

  void deref( signal_t f )    /*recursive function in order to deref all the children when derefferecing the node*/                                                                                                  
  {
	if (nodes[get_index(f)].ref_count != 0) {
    nodes[get_index(f)].ref_count = nodes[f>>1].ref_count - 1 ;
    }
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
	 assert( ( get_index(f) < nodes.size()) && "Make sure f exists." );
	return f ^ 0x1;
  }

  /* Compute f ^ g */
  signal_t XOR( signal_t f, signal_t g )
  {
    assert( get_index(f) < nodes.size() && "Make sure f exists." );
    assert( get_index(g) < nodes.size() && "Make sure g exists." );
    ++num_invoke_xor;
	
	
	bool f_is_complemented = is_complemented(f);
	bool g_is_complemented = is_complemented(g);
	const auto it = computed_table_XOR.find( std::make_tuple(f ,g ));
    if ( it != computed_table_XOR.end() ){
	    return it-> second ;
	}
	
	const auto it1 = computed_table_XOR.find( std::make_tuple(g ,f ));
    if ( it1 != computed_table_XOR.end() ){
	    return it1-> second ;
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

    Node const& F = nodes[get_index(f)];
    Node const& G = nodes[get_index(g)];
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T & (~0x1) : F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T & (~0x1) : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T & (~0x1) : F.T;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T & (~0x1) : G.T;
    }
	
    signal_t const r0 = XOR( f0, g0 );
    signal_t const r1 = XOR( f1, g1 );
    signal_t RESULTAT=unique( x, r1, r0 );
    computed_table_XOR.emplace(std::make_tuple(f,g) , RESULTAT );
    return RESULTAT;
  }

  /* Compute f & g */
  signal_t AND( signal_t f, signal_t g )
  {
	  
    assert( get_index(f) < nodes.size() && "Make sure f exists." );
    assert( get_index(g) < nodes.size() && "Make sure g exists." );
    ++num_invoke_and;
   
	
	bool f_is_complemented = is_complemented(f);
	bool g_is_complemented = is_complemented(g);
	
	
	const auto it = computed_table_AND.find( std::make_tuple(f ,g ));
    if ( it != computed_table_AND.end() ){
	    return it-> second ;
	}
	
	const auto it1 = computed_table_AND.find( std::make_tuple(g ,f ));
    if ( it1 != computed_table_AND.end() ){
	    return it1-> second ;
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

    Node const& F = nodes[get_index(f)];
    Node const& G = nodes[get_index(g)];
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T ^ 0x1 : F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T ^ 0x1 : F.T;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
    }
	//std::cout << "f0 = " << f0 << " g0 = " << g0 << " f1 = " << f1 << " g1 = " << g1 << std::endl;
    signal_t const r0 = AND( f0, g0 );
    signal_t const r1 = AND( f1, g1 );
    signal_t RESULTAT=unique( x, r1, r0 );
    computed_table_AND.emplace(std::make_tuple(f,g) , RESULTAT );
    return RESULTAT;
  }
  
  /* Compute f | g */
  signal_t OR( signal_t f, signal_t g )
  {
	++num_invoke_or;
    assert( get_index(f) < nodes.size() && "Make sure f exists." );
    assert( get_index(g) < nodes.size() && "Make sure g exists." );
    
    
	
	bool f_is_complemented = is_complemented(f);
	bool g_is_complemented = is_complemented(g);
	
		
	const auto it = computed_table_OR.find( std::make_tuple(f ,g ));
    if ( it != computed_table_OR.end() ){
	    return it-> second ;
	}
	
	const auto it1 = computed_table_OR.find( std::make_tuple(g ,f ));
    if ( it1 != computed_table_OR.end() ){
	    return it1-> second ;
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

    Node const& F = nodes[get_index(f)];
    Node const& G = nodes[get_index(g)];
    var_t x;
    signal_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T ^ 0x1 : F.T;
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T ^ 0x1 : F.T;
      g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
      g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
    }

    signal_t const r0 = OR( f0, g0 );
    signal_t const r1 = OR( f1, g1 );
    signal_t RESULTAT=unique( x, r1, r0 );
    computed_table_OR.emplace(std::make_tuple(f,g) , RESULTAT );
    return RESULTAT;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  signal_t ITE( signal_t f, signal_t g, signal_t h )
  {
	++num_invoke_ite;
    assert( get_index(f) < nodes.size() && "Make sure f exists." );
    assert( get_index(g) < nodes.size() && "Make sure g exists." );
    assert( get_index(h) < nodes.size() && "Make sure h exists." );
    
    
	
	bool f_is_complemented = is_complemented(f);
	bool g_is_complemented = is_complemented(g);
	bool h_is_complemented = is_complemented(h);
		
	const auto it = computed_table_ITE.find( std::make_tuple(f ,g ,h ));
    if ( it != computed_table_ITE.end() ){
	    return it-> second ;
	}
	
	const auto it1 = computed_table_ITE.find( std::make_tuple(f ^ 0x1,h ,g ));
    if ( it1 != computed_table_ITE.end() ){
	    return it1-> second ;
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

    Node const& F = nodes[get_index(f)];
    Node const& G = nodes[get_index(g)];
    Node const& H = nodes[get_index(h)];
    var_t x;
    signal_t f0, f1, g0, g1, h0, h1;
    if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
    {
      x = F.v;
      f0 = f_is_complemented ? F.E ^ 0x1 : F.E;
      f1 = f_is_complemented ? F.T ^ 0x1 : F.T;
      if ( G.v == F.v )
      {
        g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
        g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
      }
      else
      {
        g0 = g1 = g;
      }
      if ( H.v == F.v )
      {
        h0 = h_is_complemented ? H.E ^ 0x1 : H.E;
        h1 = h_is_complemented ? H.T ^ 0x1 : H.T;
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
        g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
        g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
        h0 = h1 = h;
      }
      else if ( H.v < G.v )
      {
        x = H.v;
        g0 = g1 = g;
        h0 = h_is_complemented ? H.E ^ 0x1 : H.E;
        h1 = h_is_complemented ? H.T ^ 0x1 : H.T;
      }
      else /* G.v == H.v */
      {
        x = G.v;
        g0 = g_is_complemented ? G.E ^ 0x1 : G.E;
        g1 = g_is_complemented ? G.T ^ 0x1 : G.T;
        h0 = h_is_complemented ? H.E ^ 0x1 : H.E;
        h1 = h_is_complemented ? H.T ^ 0x1 : H.T;
      }
    }

    signal_t const r0 = ITE( f0, g0, h0 );
    signal_t const r1 = ITE( f1, g1, h1 );
    signal_t RESULTAT=unique( x, r1, r0 );
    computed_table_ITE.emplace(std::make_tuple(f,g,h) , RESULTAT );
    return RESULTAT;
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
  

  /* Get the truth table of the BDD rooted at node f. */
  Truth_Table get_tt( signal_t f ) const
  {
    assert( (f>>1) < nodes.size() && "Make sure f exists." );
    //assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );

    if ( f == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    bool node_is_complemented = f & 0x1;
    bool Else_is_complemented = nodes[get_index(f)].E & 0x1;
    
    var_t const x = nodes[get_index(f)].v;
    // std::cout << "enter1_get_tt" << std::endl;
    signal_t const fx = nodes[get_index(f)].T;
    // std::cout << "enter2_get_tt" << std::endl;
    signal_t const fnx = nodes[get_index(f)].E;
    // std::cout << "enter3_get_tt" << std::endl;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    //std::cout << "enter4_get_tt" << std::endl;
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    // std::cout << "enter5_get_tt" << std::endl;
  
    if (not node_is_complemented and not Else_is_complemented)
		return ( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & get_tt( fnx & (~0x1) ) );
		
	if (not node_is_complemented and  Else_is_complemented)
		return ( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & ~get_tt( fnx & (~0x1) ) );
		
	if (node_is_complemented and  Else_is_complemented)
		return ~(( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & ~get_tt( fnx & (~0x1) ) ));
		
	if ( node_is_complemented and not Else_is_complemented)
		return ~(( tt_x & get_tt( fx & (~0x1) ) ) | ( ~ tt_x & get_tt( fnx & (~0x1) ) ));
		return 0 ;
		
  }

  /* Whether `f` is dead (having a reference count of 0). */
  bool is_dead( index_t f ) const                                                                                               
  {
    if (nodes[f].ref_count == 0 ) {
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
    assert( ( f >> 1 ) < nodes.size() && "Make sure f exists." );

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
    assert( ( (get_index(F.T)) >> 1 ) < nodes.size() && "Make sure the children exist." );
    assert( ( (get_index(F.E)) >> 1 ) < nodes.size() && "Make sure the children exist." );
    if ( !visited[get_index(F.T)] )
    {
		
      n += num_nodes_rec( F.T, visited );
      visited[get_index(F.T)] = true;
    }
    if ( !visited[get_index(F.E)] )
    {
		
      n += num_nodes_rec( F.E, visited );
      visited[get_index(F.E)] = true;
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
