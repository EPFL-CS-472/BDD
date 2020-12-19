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

//Hash function for tuples (ITE)
template<>
struct hash<tuple<uint32_t, uint32_t, uint32_t>>
{
  using argument_type = tuple<uint32_t, uint32_t, uint32_t>;
  using result_type = size_t;
  result_type operator() ( argument_type const& in ) const
  {
    result_type seed = 0;
    hash_combine( seed, get<0>(in) );
    hash_combine( seed, get<1>(in) );
    hash_combine( seed, get<2>(in) );
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
    index_t E; /* index of ELSE child ... WARNING : ELSE and THEN edges use their LSB as the complemented bit, so as to avoid using blocks of memory for just a boolean (inspired from an internet paper I can't find again)*/
    uint32_t nref; //Reference count
  };
  
  //No need to store NOT as it is so trivial that looking up the table would cost more than recomputing it
  //No need to store AND as De Morgan's laws allow to compute them from OR table
  enum ops : uint32_t {	
  	ops_xor,
  	ops_or,
  	ops_ite,
  	num_ops
  };

public:
  explicit BDD( uint32_t num_vars )
    : unique_table( num_vars ), num_invoke_not( 0u ), num_invoke_and( 0u ), num_invoke_or( 0u ), 
      num_invoke_xor( 0u ), num_invoke_ite( 0u )
  {
    nodes.emplace_back( Node({num_vars, 0, 0}) ); /* constant 1 */
    nodes[0].nref = 0;	//Initialize the ref count at 0 for the node
    /* `nodes` is initialized with two `Node`s representing the terminal (constant) nodes.
     * Their `v` is `num_vars` and their indices are 0 and 1.
     * (Note that the real variables range from 0 to `num_vars - 1`.)
     * Both of their children point to themselves, just for convenient representation.
     *
     * `unique_table` is initialized with `num_vars` empty maps. */
  }
  
  //Referencing function
  index_t ref(index_t f){
  	index_t fb = f>>1;
  	assert(fb < nodes.size() && "Make sure the node exists");
  	nodes[(f>>1)].nref++;
  	return f;
  }
  
  //Dereferencing function
  void deref(index_t f){
  	index_t fb = f>>1;
  	assert(fb < nodes.size() && "Make sure the node exists");
  	nodes[fb].nref--;
  	if(is_dead(f) && f > 1){	//Deref the children if the node is dead (Constant can't be dereffed)
  		deref(nodes[fb].T);
  		deref(nodes[fb].E);
  	}
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
    return value ? 0 : 1; //Constant 1 is at position 0, constant 0 too but the LSB is set to 1 because it is the complement of constant 1
  }

  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  index_t unique( var_t var, index_t T, index_t E )
  {
    index_t Tb = T >> 1; //Get rid of the complement bit
    index_t Eb = E >> 1;
    assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." );
    assert( Tb < nodes.size() && "Make sure the children exist." );
    assert( Eb < nodes.size() && "Make sure the children exist." );
    assert( nodes[Tb].v > var && "With static variable order, children can only be below the node." );
    assert( nodes[Eb].v > var && "With static variable order, children can only be below the node." );

    /* Reduction rule: Identical children */
    if ( T == E )
    {
      return T;
    }
    
    bool negT = T & 0x1;
    //Check if THEN is complemented. If so, uncomplement it and pass to the equivalent canonical form
    if(negT){
    	T = NOT(T);
    	E = NOT(E);
    }

    /* Look up in the unique table. */
    const auto it = unique_table[var].find( {T, E} );
    if ( it != unique_table[var].end() )
    {
      /* The required node already exists. Return it. */
      return negT ? ((it->second)<<1)|0x1 : (it->second)<<1; //Complementation in canonical form
    }
    else
    {
      //Garbage collection
      if(nodes.size() > threshold_nodes_size){
      	if(num_nodes() < 9.9*nodes.size()/10) garbage_collector(); //If more than 10% dead nodes, garbage collection is enabled
      }
      
      /* Create a new node and insert it to the unique table. */
      index_t new_index;
      /*if(!recycled_nodes.empty()){
      	new_index = recycled_nodes.back();
      	recycled_nodes.pop_back();
      	nodes[new_index] = Node({var, ref(T), ref(E)});
      }else{*/
      	new_index = nodes.size();
      	nodes.emplace_back( Node({var, ref(T), ref(E)}) );
      //}
    
      nodes[new_index].nref = 0;	//Initialize the ref count at 0 for the new node
      unique_table[var][{T, E}] = new_index;
      return negT ? ((new_index)<<1)|0x1 : (new_index)<<1; //Complementation in canonical form
    }
  }

  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
  index_t literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ) );
  }

  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
  index_t NOT( index_t f )
  {
    ++num_invoke_not;
    assert( (f>>1) < nodes.size() && "Make sure f exists." );
    return (f & 0x1) ? f & 0xfffffffe : f | 0x1;
  }

  /* Compute f ^ g */
  index_t XOR( index_t f, index_t g )
  {
    ++num_invoke_xor;
    index_t fb = f>>1;
    index_t gb = g>>1;
    bool negf = f & 0x1;
    bool negg = g & 0x1;
    assert( fb < nodes.size() && "Make sure f exists." );
    assert( gb < nodes.size() && "Make sure g exists." );
    constexpr ops op = ops::ops_xor;

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
    
    //Management of equivalences
    //F^G = ~F^~G = ~(~F^G) = ~(F^~G)
    index_t look1, look2;
    if(negf) look1 = NOT(f);
    else look1 = f;
    if(negg) look2 = NOT(g);
    else look2 = g;
    
    //Cache lookup
    const auto it = computed_tables.at(op).find(order(look1, look2));	//order for Commutation
    if(it != computed_tables.at(op).end()){
    	return (negf^negg) ? NOT(it->second) : it->second;
    }

    Node const& F = nodes[fb];
    Node const& G = nodes[gb];
    var_t x;
    index_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }

    index_t const r0 = ref(XOR( f0, g0 ));
    index_t const r1 = ref(XOR( f1, g1 ));
    index_t ret = unique( x, r1, r0 );
    computed_tables.at(op)[order(look1, look2)] = (negf^negg) ? NOT(ret) : ret;
    deref(r0);
    deref(r1);
    return ret;
  }

  /* Compute f & g */
  index_t AND( index_t f, index_t g )
  {
    ++num_invoke_and;
    index_t fb = f >> 1;
    index_t gb = g >> 1;
    bool negf = f & 0x1;
    bool negg = g & 0x1;
    assert( fb < nodes.size() && "Make sure f exists." );
    assert( gb < nodes.size() && "Make sure g exists." );
    constexpr ops op = ops::ops_or; //OR because it's the same as AND with De Morgan's

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
    
    //Management of equivalences (De Morgan's laws)
    index_t look1, look2;
    look1 = NOT(f);
    look2 = NOT(g);

    //Cache lookup
    const auto it = computed_tables.at(op).find(order(look1, look2));	//Order for Commutation
    if(it != computed_tables.at(op).end()){
    	return NOT(it->second);
    }

    Node const& F = nodes[fb];
    Node const& G = nodes[gb];
    var_t x;
    index_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }

    index_t const r0 = ref(AND( f0, g0 ));
    index_t const r1 = ref(AND( f1, g1 ));
    index_t ret = unique( x, r1, r0 );
    computed_tables.at(op)[order(look1, look2)] = NOT(ret);
    deref(r0);
    deref(r1);
    return ret;
  }

  /* Compute f | g */
  index_t OR( index_t f, index_t g )
  {
    ++num_invoke_or;
    index_t fb = f >> 1;
    index_t gb = g >> 1;
    bool negf = f & 0x1;
    bool negg = g & 0x1;
    assert( fb < nodes.size() && "Make sure f exists." );
    assert( gb < nodes.size() && "Make sure g exists." );
    constexpr ops op = ops::ops_or;

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
    
    //Cache lookup
    const auto it = computed_tables.at(op).find(order(f, g));	//No need to manage equivalences here, it's already done in the AND part
    if(it != computed_tables.at(op).end()){
    	return it->second;	//TODO resurrection ?
    }

    Node const& F = nodes[fb];
    Node const& G = nodes[gb];
    var_t x;
    index_t f0, f1, g0, g1;
    if ( F.v < G.v ) /* F is on top of G */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      g0 = g1 = g;
    }
    else if ( G.v < F.v ) /* G is on top of F */
    {
      x = G.v;
      f0 = f1 = f;
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }
    else /* F and G are at the same level */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
    }

    index_t const r0 = ref(OR( f0, g0 ));
    index_t const r1 = ref(OR( f1, g1 ));
    index_t ret = unique( x, r1, r0 );
    computed_tables.at(op)[order(f, g)] = ret;
    deref(r0);
    deref(r1);
    return ret;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
  index_t ITE( index_t f, index_t g, index_t h )
  {
    ++num_invoke_ite;
    index_t fb = f >> 1;
    index_t gb = g >> 1;
    index_t hb = h >> 1;
    bool negf = f & 0x1;
    bool negg = g & 0x1;
    bool negh = h & 0x1;
    assert( fb < nodes.size() && "Make sure f exists." );
    assert( gb < nodes.size() && "Make sure g exists." );
    assert( hb < nodes.size() && "Make sure h exists." );
    constexpr ops op = ops::ops_ite;

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
    if(g == constant(true) && h == constant (false)) //A couple more terminal cases
    {
    	return f;
    }
    if(g == constant(false) && h == constant (true))
    {
    	return NOT(f);
    }
    
    //Management of equivalences
    //FGH = ~FGH = ~(F~G~H) = ~(~F~G~H)
    //F~GH = ~FH~G = ~(FG~H) = ~(~F~HG)
    index_t look1, look2, look3;
    if(negf){
    	look1 = NOT(f);
    	look2 = h;
    	look3 = g;
    }else{
    	look1 = f;
    	look2 = g;
    	look3 = h;
    }
    if(look3 & 0x1){
    	look2 = NOT(look2);
    	look3 = NOT(look3);
    }
    
    //Cache lookup
    const auto it = computed_tables.at(op).find({look1, look2, look3});
    if(it != computed_tables.at(op).end()){
    	return (look3 & 0x1) ? NOT(it->second) : it->second;
    }

    Node const& F = nodes[fb];
    Node const& G = nodes[gb];
    Node const& H = nodes[hb];
    var_t x;
    index_t f0, f1, g0, g1, h0, h1;
    if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
    {
      x = F.v;
      if(negf){
      	f0 = NOT(F.E);
      	f1 = NOT(F.T);
      }else{
      	f0 = F.E;
      	f1 = F.T;
      }
      if ( G.v == F.v )
      {
      	if(negg){
      		g0 = NOT(G.E);
      		g1 = NOT(G.T);
      	}else{
      		g0 = G.E;
      		g1 = G.T;
      	}
      }
      else
      {
        g0 = g1 = g;
      }
      if ( H.v == F.v )
      {
        if(negh){
      		h0 = NOT(H.E);
      		h1 = NOT(H.T);
      	}else{
      		h0 = H.E;
      		h1 = H.T;
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
      if(negg){
      	g0 = NOT(G.E);
      	g1 = NOT(G.T);
      }else{
      	g0 = G.E;
      	g1 = G.T;
      }
        h0 = h1 = h;
      }
      else if ( H.v < G.v )
      {
        x = H.v;
        g0 = g1 = g;
        if(negh){
      		h0 = NOT(H.E);
      		h1 = NOT(H.T);
      	}else{
      		h0 = H.E;
      		h1 = H.T;
      	}
      }
      else /* G.v == H.v */
      {
        x = G.v;
        if(negg){
      		g0 = NOT(G.E);
      		g1 = NOT(G.T);
     	}else{
      		g0 = G.E;
      		g1 = G.T;
      	}
        if(negh){
      		h0 = NOT(H.E);
      		h1 = NOT(H.T);
        }else{
      		h0 = H.E;
      		h1 = H.T;
      	}
      }
    }

    index_t const r0 = ref(ITE( f0, g0, h0 ));
    index_t const r1 = ref(ITE( f1, g1, h1 ));
    index_t ret = unique( x, r1, r0 );
    computed_tables.at(op)[{look1, look2, look3}] = (look3 & 0x1) ? NOT(ret) : ret;
    deref(r0);
    deref(r1);
    return ret;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( index_t f, std::ostream& os = std::cout ) const
  {
    index_t fb = f >> 1;
    for ( auto i = 0u; i < nodes[fb].v; ++i )
    {
      os << "  ";
    }
    if ( f == 1 )
    {
      os << "node " << f << ": constant " << 0 << std::endl;
    }
    else if(f == 0){
      os << "node " << f << ": constant " << 1 << std::endl;
    }else{
      os << ((f & 0x1) ? "~" : " ") << "node " << f << ": var = " << nodes[fb].v << ", T = " << nodes[fb].T 
         << ", E = " << nodes[fb].E << std::endl;
      for ( auto i = 0u; i < nodes[fb].v; ++i )
      {
        os << "  ";
      }
      os << "> THEN branch" << std::endl;
      print( nodes[fb].T, os );
      for ( auto i = 0u; i < nodes[fb].v; ++i )
      {
        os << "  ";
      }
      os << "> ELSE branch" << std::endl;
      print( nodes[fb].E, os );
    }
  }

  /* Get the truth table of the BDD rooted at node f. */
  Truth_Table get_tt( index_t f ) const
  {
    index_t fb = f >> 1;
    assert( fb < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }
    
    /* Shannon expansion: f = x f_x + x' f_x' */
    var_t const x = nodes[fb].v;
    index_t const fx = nodes[fb].T;
    index_t const fnx = nodes[fb].E;
    Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
    Truth_Table ret = ( tt_x & get_tt( fx ) ) | ( tt_nx & get_tt( fnx ) );
    return (f & 0x1) ? ~ret : ret;
  }

  /* Whether `f` is dead (having a reference count of 0). */
  bool is_dead( index_t f ) const
  {
    return nodes[(f>>1)].nref == 0;
  }
  
  //Garbage collection (inspired from the ZDD package (Bill library))
  void garbage_collector(){
  	//First purge the computed cache
  	for(auto a = 0; a < ops::num_ops; a++){
  		for(auto it = computed_tables.at(a).begin(); it != computed_tables.at(a).end();){
  			auto f = std::get<0>(it -> first);
  			auto g = std::get<1>(it -> first);
  			if(is_dead(it->second) || is_dead(f) || is_dead(g)) it = computed_tables.at(a).erase(it); //If any of the nodes is dead, remove the table entry
  			else it++;
  		}
  	}
  	
  	//Then purge the unique table and Nodes vector
    	for ( auto i = 1u; i < nodes.size(); ++i ) {
  		Node node = nodes[i];
  		if(node.nref == 0){
  			auto const it = unique_table[node.v].find({node.T, node.E});
  			if(it != unique_table[node.v].end()){
  				unique_table[node.v].erase(it);
  				recycled_nodes.emplace_back(i);	//Tag the node as recycled for further use
  		 	}
  		}
  	}
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 1u; i < nodes.size(); ++i ) 
    {
      if ( !is_dead( i<<1 ) )
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( index_t f ) const
  {
    assert( (f>>1) < nodes.size() && "Make sure f exists." );

    if ( f == constant( false ) || f == constant( true ) )
    {
      return 0u;
    }

    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    visited[1] = true;

    return num_nodes_rec( f, visited ); //modify ??
  }

  uint64_t num_invoke() const
  {
    //return num_invoke_not + num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
    return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
  }

private:
  /**********************************************************/
  /******************** Helper Functions ********************/
  /**********************************************************/

  uint64_t num_nodes_rec( index_t f, std::vector<bool>& visited ) const
  {
    index_t fb = f>>1;
    assert( fb < nodes.size() && "Make sure f exists." );
    

    uint64_t n = 0u;
    Node const& F = nodes[fb];
    index_t Ftb = F.T >> 1;
    index_t Feb = F.E >> 1;
    assert( Ftb < nodes.size() && "Make sure the children exist." );
    assert( Feb < nodes.size() && "Make sure the children exist." );
    if ( !visited[Ftb] )
    {
      n += num_nodes_rec( F.T, visited );
      visited[Ftb] = true;
    }
    if ( !visited[Feb] )
    {
      n += num_nodes_rec( F.E, visited );
      visited[Feb] = true;
    }
    return n + 1u;
  }
  
  //Used for symmetrization of unordered map find()
  std::tuple<index_t, index_t, index_t> order(index_t a, index_t b){
  	index_t big = std::max(a, b);
  	index_t low = std::min(a, b);
  	return {big, low, low};
  }

private:
  std::vector<Node> nodes;
  std::vector<index_t> recycled_nodes;
  std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table;
  /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
   * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
   * See the implementation of `unique` for example usage. */
   std::array<std::unordered_map<std::tuple<index_t, index_t, index_t>, index_t>, ops::num_ops> computed_tables;
   const int threshold_nodes_size = 1; //Threshold over which we start looking for garbage collection

  /* statistics */
  uint64_t num_invoke_not, num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
};
