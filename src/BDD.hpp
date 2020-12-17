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


// Hash table with 3 elements required for ITE computed table
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

    index_t parent_not_comp; // parent edge 1
    index_t parent_comp; // parent edge 0

    uint32_t cnt_ref; // reference count : number of edges pointing to the node
  };

  struct Node_comp     // new structure to deal with complemented edges
  {
    bool complemented;
    index_t i_comp;
  };

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_and( 0u ), num_invoke_or( 0u ),
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back(Node({num_vars, 1, 1, 1, 0, 0})); /* constant 1 */

    nodes_comp.emplace_back(Node_comp{true, 0}); // constant node 0 is complemented
    nodes_comp.emplace_back(Node_comp{false, 0}); // constant node 1 is not complemented

    /* `nodes` is initialized with two `Node`s representing the terminal (constant) nodes.
     * Their `v` is `num_vars` and their indices are 0 and 1.
     * (Note that the real variables range from 0 to `num_vars - 1`.)
     * Both of their children point to themselves, just for convenient representation.
     
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
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( T < nodes_comp.size() && "Make sure the children exist." );
    assert( E < nodes_comp.size() && "Make sure the children exist." );
    assert( nodes[nodes_comp[T].i_comp].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[nodes_comp[E].i_comp].v > var && "With static variable order, children can only be below the node." );

    /* Reduction rule: Identical children */

    //std::cout << "nodes= "  ;

    //for (auto i = 0 ; i < nodes.size() ; i++ )
	//{ 
	//  std::cout << "i= " << i << " , " ; 
        //  std::cout << "var=" << nodes[i].v << " , " ;
      	//  std::cout << "T=" << nodes[i].T << " , " ;
        //  std::cout << "E=" << nodes[i].E << " , " ;
	//  std::cout << "p1=" << nodes[i].parent_not_comp << " , " ;
	//  std::cout << "p0=" << nodes[i].parent_comp  << " , ";
	//  std::cout << "ref=" << nodes[i].cnt_ref << std::endl;
	//}

    //std::cout << "nodes_comp= "  ;

    //for (auto i = 0 ; i < nodes_comp.size() ; i++ )
	//{ 
	//  std::cout << "i= " << i << " , " ; 
        //  std::cout << "comp=" << nodes_comp[i].complemented << " , " ;
      	//  std::cout << "i_comp=" << nodes_comp[i].i_comp << std::endl ;

	//}

    if ( T == E )
    {
      return ref(T);
    }
    
    bool Is_T_complemented = false; 
    
    if(nodes_comp[T].complemented)  
    {
      Is_T_complemented = true; // T edge complemented ?
      T = Store_complement(false, nodes_comp[T].i_comp);  // remove THEN edge complement and complement ELSE instead
      E = Reverse_complementation(E);			  // remove THEN edge complement and complement ELSE instead
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T, E} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */

      return Store_complement(Is_T_complemented, it->second);
    }
    else
    {
      /* Create a new node and insert it to the unique table. */
      index_t const new_index = nodes.size();
      nodes.emplace_back( Node({var, T, E, 0xFFFFFFFF,0xFFFFFFFF, 0}) ); // 0xFFFFFFFF indicates the node is newly created and parents 
      unique_table[var][{T, E}] = new_index;                             // and have not been modified
      ref(T);								 // reference children since new node has been created
      ref(E);
      return Store_complement(Is_T_complemented, new_index);
    }
  }



/* Increase the reference count */ 
  index_t ref(index_t f){

    nodes[nodes_comp[f].i_comp].cnt_ref ++  ;

    return f;

  }

/* Decrease the reference count */ 
  void deref(index_t f){
    
    nodes[nodes_comp[f].i_comp].cnt_ref -- ;

    if (nodes[nodes_comp[f].i_comp].cnt_ref == 0)
      {
	deref( nodes[nodes_comp[f].i_comp].T ) ;
	deref( nodes[nodes_comp[f].i_comp].E ) ;
      }

  }


  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  index_t literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ));
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
  index_t NOT( index_t f )
  {
 
    return Reverse_complementation(f); // return node with complemented edge 
  }

  /* Compute f ^ g */
  index_t XOR( index_t f, index_t g )
  {
    assert( f < nodes_comp.size() && "Make sure f exists." );
    assert( g < nodes_comp.size() && "Make sure g exists." );

    ++num_invoke_xor;

    index_t res = 0 ;

    /* trivial cases */
    if ( f == g )
    {
      res = constant( false );
    } 
    else if ( f == constant( false ) )
    {
      res = g;
    } 
    else if ( g == constant( false ) )
    {
      res = f;
    } 
    else if ( f == constant( true ) )
    {
      res = NOT( g );
    } 
    else if ( g == constant( true ) )
    {
      res = NOT( f );
    } 
    else if ( f == NOT( g ) )
    {
      res = constant( true );
    } 
    else {
    

      Node const& F = nodes[nodes_comp[f].i_comp];
      Node const& G = nodes[nodes_comp[g].i_comp];

      var_t x;
      index_t f0, f1, g0, g1;

      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;

	if (nodes_comp[f].complemented == true)
	{
	  f0 = Reverse_complementation(F.E) ;
	}
	
	else
	{
	  f0 = F.E ;
	}

	if (nodes_comp[f].complemented == true)
	{
	  f1 = Reverse_complementation(F.T) ;
	}
	
	else
	{
	  f1 = F.T ;
	} 

        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;

	if (nodes_comp[g].complemented == true)
	{
	  g0 = Reverse_complementation(G.E) ;
	}
	
	else
	{
	  g0 = G.E ;
	}

	if (nodes_comp[g].complemented == true)
	{
	  g1 = Reverse_complementation(G.T) ;
	}
	
	else
	{
	  g1 = G.T ;
	} 

      }
      else /* F and G are at the same level */
      {
        x = F.v;

	if (nodes_comp[f].complemented == true)
	{
	  f0 = Reverse_complementation(F.E) ;
	}
	
	else
	{
	  f0 = F.E ;
	}

	if (nodes_comp[f].complemented == true)
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	if (nodes_comp[g].complemented == true)
	{
	  g0 = Reverse_complementation(G.E) ;
	}
	
	else
	{
	  g0 = G.E ;
	}

	if (nodes_comp[g].complemented == true)
	{
	  g1 = Reverse_complementation(G.T) ;
	}
	
	else
	{
	  g1 = G.T ;
	} 

      }

      // Look if XOR(f,g) has already been computed in XOR computed table

      index_t r0, r1;

      const auto it0 = cmp_tab_XOR.find( std::make_pair(f0,g0) ) ;

      if(it0 != cmp_tab_XOR.end())
      { 
        r0 = it0->second ;
      }
      else
      {
	r0 = XOR(f0,g0);
      }
      
      const auto it1 = cmp_tab_XOR.find( std::make_pair(f1,g1) ) ;

      if(it1 != cmp_tab_XOR.end())
      { 
        r1 = it1->second ;
      }
      else
      {
	r1 = XOR(f1,g1);
      }

      res = unique(x, r1, r0);
      
      }

      // Add results to computed tables

     cmp_tab_XOR[std::make_pair(f, g)] = res  ; // f^g
     cmp_tab_XOR[std::make_pair(Reverse_complementation(f), g)] = Reverse_complementation(res)  ; //f'^g = (f^g)'
     cmp_tab_XOR[std::make_pair(f, Reverse_complementation(g))] = Reverse_complementation(res)  ; //f^g' = (f^g)'
     cmp_tab_XOR[std::make_pair(Reverse_complementation(f), Reverse_complementation(g))] = Reverse_complementation(res)  ; // f'^g' = (f^g)'

    return res;
  }

  /* Compute f & g */
  index_t AND( index_t f, index_t g )
  {
    assert( f < nodes_comp.size() && "Make sure f exists." );
    assert( g < nodes_comp.size() && "Make sure g exists." );
    ++num_invoke_and;

    index_t res ;

    /* trivial cases */
    if ( f == constant( false ) || g == constant( false ) )
    {
      res = constant( false );
    }
    else if ( f == constant( true ) )
    {
      res = g;
    } 
    else if ( g == constant( true ) )
    {
      res = f;
    }
    else if ( f == g )
    {
      res = f;
    } 
    else{

      Node const& F = nodes[nodes_comp[f].i_comp];
      Node const& G = nodes[nodes_comp[g].i_comp];

      var_t x;
      index_t f0, f1, g0, g1;

      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
    
        if (nodes_comp[f].complemented)
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else
	{
	  f0 = F.E ;
	}
        g0 = g1 = g;
      }

      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;

	if (nodes_comp[f].complemented)
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else 
	{
	  f0 = F.E ;
	}

	if (nodes_comp[f].complemented)
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	else 
	{
	  f1 = F.T ;
	}
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;

	if (nodes_comp[g].complemented)
	{
	  g0 = Reverse_complementation(G.E) ;
	}

	else 
	{
	  g0 = G.E ;
	}

	if (nodes_comp[g].complemented)
	{
	  g1 = Reverse_complementation(G.T) ;
	}

	else 
	{
	  g1 = G.T ;
	}
      }
      else /* F and G are at the same level */
      {
        x = F.v;

	if (nodes_comp[f].complemented)
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else 
	{
	  f0 = F.E ;
	}

	if (nodes_comp[f].complemented)
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	else 
	{
	  f1 = F.T ;
	}

	if (nodes_comp[g].complemented)
	{
	  g0 = Reverse_complementation(G.E) ;
	}

	else 
	{
	  g0 = G.E ;
	}

	if (nodes_comp[g].complemented)
	{
	  g1 = Reverse_complementation(G.T) ;
	}

	else 
	{
	  g1 = G.T ;
	}
      }

      
      // Look if AND(f,g) has already been computed in AND computed table

      index_t r0, r1;

      const auto it0 = cmp_tab_AND.find( std::make_pair(f0,g0) ) ;

      if(it0 != cmp_tab_AND.end())
      { 
        r0 = it0->second ;
      }
      else
      {
	r0 = AND(f0,g0);
      }
      
      const auto it1 = cmp_tab_AND.find( std::make_pair(f1,g1) ) ;

      if(it1 != cmp_tab_AND.end())
      { 
        r1 = it1->second ;
      }
      else
      {
	r1 = AND(f1,g1);
      }

      res = unique(x, r1, r0);

    }

    // Add results to computed tables 

    cmp_tab_AND[std::make_pair(f, g)] = res  ; // f&g
    cmp_tab_OR[std::make_pair(Reverse_complementation(f), Reverse_complementation(g))] = Reverse_complementation(res)  ;  // f'|g' = (f&g)'

    return res;
  }


  /* Compute f | g */
  index_t OR( index_t f, index_t g )
  {
    assert( f < nodes_comp.size() && "Make sure f exists." );
    assert( g < nodes_comp.size() && "Make sure g exists." );
    ++num_invoke_or;

    index_t res;

    /* trivial cases */
    if ( f == constant( true ) || g == constant( true ) )
    {
      res = constant( true );
    }
    else if ( f == constant( false ) )
    {
      res = g;
    } 
    else if ( g == constant( false ) )
    {
      res = f;
    } 
    else if ( f == g ) 
    {
      res = f;
    } 
    else {
      Node_comp rF = nodes_comp[f];
      Node_comp rG = nodes_comp[g];

      Node const& F = nodes[nodes_comp[f].i_comp];
      Node const& G = nodes[nodes_comp[g].i_comp];

      var_t x;
      index_t f0, f1, g0, g1;

      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;

	if ( nodes_comp[f].complemented == true )
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else
	{
	  f0 = F.E ;
	}

	if ( nodes_comp[f].complemented == true )
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	else
	{
	  f1 = F.T ;
	}
        g0 = g1 = g;
      }

      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;

	if ( nodes_comp[g].complemented == true )
	{
	  g0 = Reverse_complementation(G.E) ;
	}

	else
	{
	  g0 = G.E ;
	}

	if ( nodes_comp[g].complemented == true )
	{
	  g1 = Reverse_complementation(G.T) ;
	}

	else
	{
	  g1 = G.T ;
	}
      }
      else /* F and G are at the same level */
      {
        x = F.v;

	if ( nodes_comp[f].complemented == true )
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else
	{
	  f0 = F.E ;
	}

	if ( nodes_comp[f].complemented == true )
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	else
	{
	  f1 = F.T ;
	}

	if ( nodes_comp[g].complemented == true )
	{
	  g0 = Reverse_complementation(G.E) ;
	}

	else
	{
	  g0 = G.E ;
	}

	if ( nodes_comp[g].complemented == true )
	{
	  g1 = Reverse_complementation(G.T) ;
	}

	else
	{
	  g1 = G.T ;
	}
      }

      // Look if OR(f,g) has already been computed in OR computed table

      index_t r0, r1;

      const auto it0 = cmp_tab_OR.find( std::make_pair(f0,g0) ) ;

      if(it0 != cmp_tab_OR.end())
      { 
        r0 = it0->second ;
      }
      else
      {
	r0 = OR(f0,g0);
      }
      
      const auto it1 = cmp_tab_OR.find( std::make_pair(f1,g1) ) ;

      if(it1 != cmp_tab_OR.end())
      { 
        r1 = it1->second ;
      }
      else
      {
	r1 = OR(f1,g1);
      }

      res = unique(x, r1, r0);

    }

    // Add results to computed tables 

    cmp_tab_OR[std::make_pair(f, g)] = res  ; // f|g
    cmp_tab_AND[std::make_pair(Reverse_complementation(f), Reverse_complementation(g))] = Reverse_complementation(res)  ; // f'& g' = (f|g)'

    return res;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  index_t ITE( index_t f, index_t g, index_t h )
  {
    assert( f < nodes_comp.size() && "Make sure f exists." );
    assert( g < nodes_comp.size() && "Make sure g exists." );
    assert( h < nodes_comp.size() && "Make sure h exists." );

    ++num_invoke_ite;

    index_t res ;
    
    /* trivial cases */
    if ( f == constant( true ) )
    {
      res = g;
    } 
    else if ( f == constant( false ) )
    {
      res = h;
    } 
    else if ( g == h )
    {
      res = g;
    } 
    else {

      Node const& F = nodes[nodes_comp[f].i_comp];
      Node const& G = nodes[nodes_comp[g].i_comp];
      Node const& H = nodes[nodes_comp[h].i_comp];

      var_t x;
      index_t f0, f1, g0, g1, h0, h1;

      if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
      {
        x = F.v;

	if ( nodes_comp[f].complemented == true )
	{
	  f0 = Reverse_complementation(F.E) ;
	}

	else
	{
	  f0 = F.E ;
	}

	if ( nodes_comp[f].complemented == true )
	{
	  f1 = Reverse_complementation(F.T) ;
	}

	else
	{
	  f1 = F.T ;
	}

        if ( G.v == F.v )
        {

	  if ( nodes_comp[g].complemented == true )
	  {
	    g0 = Reverse_complementation(G.E) ;
	  }

	  else
	  {
	    g0 = G.E ;
	  }

	  if ( nodes_comp[g].complemented == true )
	  {
	    g1 = Reverse_complementation(G.T) ;
	  }

	  else
	  {
	    g1 = G.T ;
	  }
        }


        else
        {
          g0 = g1 = g;
        }


        if ( H.v == F.v )
        {
	  if ( nodes_comp[h].complemented == true )
	  {
	    h0 = Reverse_complementation(H.E) ;
	  }

	  else
	  {
	    h0 = H.E ;
	  }

	  if ( nodes_comp[h].complemented == true )
	  {
	    h1 = Reverse_complementation(H.T) ;
	  }

	  else
	  {
	    h1 = H.T ;
	  }
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

	  if ( nodes_comp[g].complemented == true )
	  {
	    g0 = Reverse_complementation(G.E) ;
	  }

	  else
	  {
	    g0 = G.E ;
	  }

	  if ( nodes_comp[g].complemented == true )
	  {
	    g1 = Reverse_complementation(G.T) ;
	  }

	  else
	  {
	    g1 = G.T ;
	  }

          h0 = h1 = h;
        }
        else if ( H.v < G.v )
        {
          x = H.v;
          g0 = g1 = g;

	  if ( nodes_comp[h].complemented == true )
	  {
	    h0 = Reverse_complementation(H.E) ;
	  }

	  else
	  {
	    h0 = H.E ;
	  }

	  if ( nodes_comp[h].complemented == true )
	  {
	    h1 = Reverse_complementation(H.T) ;
	  }

	  else
	  {
	    h1 = H.T ;
	  }

        }
        else /* G.v == H.v */
        {
          x = G.v;

	  if ( nodes_comp[g].complemented == true )
	  {
	    g0 = Reverse_complementation(G.E) ;
	  }

	  else
	  {
	    g0 = G.E ;
	  }

	  if ( nodes_comp[g].complemented == true )
	  {
	    g1 = Reverse_complementation(G.T) ;
	  }

	  else
	  {
	    g1 = G.T ;
	  }

	  if ( nodes_comp[h].complemented == true )
	  {
	    h0 = Reverse_complementation(H.E) ;
	  }

	  else
	  {
	    h0 = H.E ;
	  }

	  if ( nodes_comp[h].complemented == true )
	  {
	    h1 = Reverse_complementation(H.T) ;
	  }

	  else
	  {
	    h1 = H.T ;
	  }

        }
      }

      // Look if ITE(f,g,h) has already been computed in ITE computed table

      index_t r0, r1;

      const auto it0 = cmp_tab_ITE.find( std::make_tuple(f0,g0,h0) ) ;

      if(it0 != cmp_tab_ITE.end())
      { 
        r0 = it0->second ;
      }
      else
      {
	r0 = ITE(f0,g0,h0);
      }
      
      const auto it1 = cmp_tab_ITE.find( std::make_tuple(f1,g1,h1) ) ;

      if(it1 != cmp_tab_ITE.end())
      { 
        r1 = it1->second ;
      }
      else
      {
	r1 = ITE(f1,g1,h1);
      }

      res = unique(x, r1, r0);

    }

    // Add results to computed tables 

    cmp_tab_ITE[std::make_tuple(f, g, h)] = res;
    cmp_tab_ITE[std::make_tuple(f, Reverse_complementation(g), Reverse_complementation(h))] = Reverse_complementation(res); // fg'+f'h' = (fg + f'h)' 
    cmp_tab_ITE[std::make_tuple(Reverse_complementation(f), h, g)] = res;
    cmp_tab_ITE[std::make_tuple(Reverse_complementation(f), Reverse_complementation(h), Reverse_complementation(g))] = Reverse_complementation(res); // f'g'+fh' = (fg + f'h)' 

    return res;
  }

/* Reverse edge complementation */
  index_t Reverse_complementation(index_t i){
    return Store_complement(!nodes_comp[i].complemented, nodes_comp[i].i_comp);
  }

  
/* Store edges complementation in nodes_comp */
  index_t Store_complement(bool complemented, index_t i_comp){ 
    if(complemented && nodes[i_comp].parent_comp != 0xFFFFFFFF )           // if the node is not newly created (!= 0xFFFFFFFF)
      return nodes[i_comp].parent_comp;					
    else if(!complemented && nodes[i_comp].parent_not_comp != 0xFFFFFFFF )
      return nodes[i_comp].parent_not_comp;
    else {						// if the node has just been created, store complementation info and return the storage index 
      auto res = nodes_comp.size(); 
      nodes_comp.emplace_back(Node_comp{complemented, i_comp});
      if(complemented)
        nodes[i_comp].parent_comp = res;
      else
        nodes[i_comp].parent_not_comp = res;
      return res;
    }
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

  /* Get the truth table of the BDD rooted at node f. */
  Truth_Table get_tt( index_t f ) const
  {
    assert( f < nodes_comp.size() && "Make sure f exists." );
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
    var_t const x = nodes[nodes_comp[f].i_comp].v;
    index_t const fx = nodes[nodes_comp[f].i_comp].T;
    index_t const fnx = nodes[nodes_comp[f].i_comp].E;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    auto res = ( tt_x & get_tt( fx ) ) | ( tt_nx & get_tt( fnx ) );
    if(nodes_comp[f].complemented)  
      return ~res;  // return complement if node complemented
    return res;
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 1u; i < nodes.size(); ++i )
    {
      if ( nodes[i].cnt_ref != 0)
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( index_t f ) const
  {
    assert(f < nodes_comp.size() && "Make sure f exists.");
    //assert( f < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) || f == constant( true ) )
    {
      return 0u;
    }

    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    visited[1] = true;

    return num_nodes_rec( nodes_comp[f].i_comp, visited );
  }

  uint64_t num_invoke() const
  {
    return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite; 
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
    assert( F.T < nodes_comp.size() && "Make sure the children exist." );
    assert( F.E < nodes_comp.size() && "Make sure the children exist." );

    if ( !visited[nodes_comp[F.T].i_comp] )
    {
      n += num_nodes_rec( nodes_comp[F.T].i_comp, visited );
      visited[nodes_comp[F.T].i_comp] = true;
    }
    if ( !visited[nodes_comp[F.E].i_comp] )
    {
      n += num_nodes_rec( nodes_comp[F.E].i_comp, visited );
      visited[nodes_comp[F.E].i_comp] = true;
    }

    return n + 1u;
  }


private:
  std::vector<Node> nodes;
  std::vector<Node_comp> nodes_comp;
  std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */


  /* computed tables for operators results storage */
  std::unordered_map<std::pair<index_t, index_t>, index_t> cmp_tab_XOR ;
  std::unordered_map<std::pair<index_t, index_t>, index_t> cmp_tab_OR ;
  std::unordered_map<std::pair<index_t, index_t>, index_t> cmp_tab_AND ;
  std::unordered_map<std::tuple<index_t, index_t, index_t>, index_t> cmp_tab_ITE ;


  /* statistics */
  uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
  
};


