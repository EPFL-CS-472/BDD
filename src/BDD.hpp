#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

/* These are just some hacks to hash std::pair (for the unique table).
 * You don't need to understand this part. EXCEPT TO BUILD NEW NEEDED HASH FUNCTIONS */
namespace std {
	template<class T>
	inline void hash_combine( size_t& seed, T const& v ) 
  	{
		seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<tuple<uint32_t, uint32_t, bool>> //Unique table modified to add the bool isComp
	{
		using argument_type = tuple<uint32_t, uint32_t, bool>;
		using result_type = size_t;
		result_type operator() ( argument_type const& in ) const 
		{
			result_type seed = 0;
			hash_combine( seed, std::get<0>(in) );
			hash_combine( seed, std::get<1>(in) );
			hash_combine( seed, std::get<2>(in) );
			return seed;
		}
	};
}

class BDD {
	public:
		using index_t = uint32_t;
		/* Declaring `index_t` as an alias for an unsigned integer.
		 * This is just for easier understanding of the code.
		 * This datatype will be used for node indices. */

		using var_t = uint32_t;
		/* Similarly, declare `var_t` also as an alias for an unsigned integer.
		 * This datatype will be used for representing variables. */
		
	private:

		struct C_Edge 
    {
			index_t ind; /* node  pointed  by the  edge */
			bool isComp; //to know if the edge is complemented or not
		};
	 
		struct Node 
    {
			var_t v; /* corresponding variable */
			C_Edge T; /* index of pointing node of  THEN child */
			C_Edge E; /* index of pointing node of ELSE child */
			bool isAlive; //is the node alive
			int count = 0; //count the number of references to this node
		};

	public:
		explicit BDD( uint32_t num_vars ) : 
			unique_table( num_vars ), num_invoke_and( 0u ), num_invoke_or( 0u ),
			num_invoke_xor( 0u ), num_invoke_ite( 0u ) 
       { 
			nodes.emplace_back( Node({num_vars, 0, 0, true}) ); /* constant 0 */
        	//nodes.emplace_back( Node({num_vars, 1, 1}) ); /* constant 1 */
			/* `nodes` is initialized with 1 `Node`s representing the terminal (positive) nodes.
			 * `v` is `num_vars` and his indice is 0.
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
		C_Edge constant( bool value ) const {
			//return value ? 1 : 0;
      		//there is only one constant node that can be complemented or not
			return ( C_Edge { 0, !value } ) ;
		}

		/* Look up (if exist) or build (if not) the node with variable `var`,
		 * THEN child `T`, and ELSE child `E`. */
	   
		C_Edge unique( var_t var, C_Edge T, C_Edge E ) 
		{
			assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." ); 
			assert( T.ind < nodes.size() && "Make sure the children exist." );
			assert( E.ind < nodes.size() && "Make sure the children exist." );
			assert( nodes[T.ind].v > var && "With static variable order, children can only be below the node." );
			assert( nodes[E.ind].v > var && "With static variable order, children can only be below the node." );
		
			//THEN should not be complemented:
			bool Tcomp = T.isComp ;
			if (T.isComp){
				T.isComp = false; 	
				E.isComp = !E.isComp;
			}
		
			/* Reduction rule: Identical children */
			//same children and ELSE not complmented then we skip this node
			if ( (T.ind == E.ind) and not(E.isComp)){
				return ( ref(C_Edge { T.ind, Tcomp }) );
			}

			/* Look up in the unique table. */
			const auto it = unique_table[var].find( std::make_tuple(T.ind, E.ind, E.isComp) );
			if ( it != unique_table[var].end() ) 
			{
				/* The required node already exists. Return it. */
				return ( C_Edge { it->second.ind, Tcomp } );
			}
		
			/* Create a new node and insert it to the unique table. */
			index_t const new_node = nodes.size();
			//std::cout << "new node index :" << new_node << std::endl;
			nodes.emplace_back( Node({var, T, E, true}) );
			unique_table[var][std::make_tuple(T.ind, E.ind, E.isComp)] = C_Edge { new_node, Tcomp } ;
			ref(T);
			ref(E);
			return ( C_Edge { new_node, Tcomp } );
	   
		}

		/* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
		C_Edge literal( var_t var, bool complement = false ) 
		{
			return unique( var, constant( !complement ), constant( complement ) );
		}
		
		
		//ref adn deref:
		C_Edge ref (C_Edge f) {
			nodes[f.ind].count++;
			return f;
		}
		void deref (C_Edge f) {
			nodes[f.ind].count--;
			if (nodes[f.ind].count == 0)//if count=0 then the node is considered as dead 
			{
				deref(nodes[f.ind].T); //so we dereference its children
				deref(nodes[f.ind].E);
				nodes[f.ind].isAlive = false;
			}
		}	  
	  
		/**********************************************************/
		/********************* BDD Operations *********************/
		/**********************************************************/

		/* Compute ~f */
		C_Edge NOT( C_Edge f ) {

			//assert( f < nodes.size() && "Make sure f exists." );
			/* trivial cases */
			//if ( f == constant( false ) )
			//{
			//return constant( true );
			//}
			//if ( f == constant( true ) )
			//{
			//return constant( false );
			//}

			//Node const& F = nodes[f];
			//var_t x = F.v;
			//index_t f0 = F.E.ind, f1 = F.T.ind;

			//C_Edge const r1 = NOT( f1 );
			//C_Edge const r0 = NOT( f0 );
			//deref(f1);
			//return unique( x, r1, r0 );
			return  ( C_Edge { f.ind, !f.isComp } );
		}

		/* Compute f ^ g */
		C_Edge XOR( C_Edge f, C_Edge g ) {			
			//assert( f < nodes.size() && "Make sure f exists." );
			//assert( g < nodes.size() && "Make sure g exists." );
			++num_invoke_xor;

			if (f.ind < g.ind){std::swap(f,g);}
			
			// computed table lookup:
			for (int i = 0; i < computed_table_XOR.size(); i++)
			{
				//XOR(f,g)
				if (computed_table_XOR[i].first  == std::make_tuple(f.ind, f.isComp, g.ind, g.isComp))
				{
					return  computed_table_XOR[i].second;
				}
			}

			/* trivial cases */
			if (f.ind==g.ind && f.isComp==g.isComp) {
				return constant( false );
			}
			if (f.ind==constant(false).ind && f.isComp==constant(false).isComp) {
				return g;
			}
			if (g.ind==constant(false).ind && g.isComp==constant(false).isComp) {
				return f;
			}
			if (f.ind==constant(true).ind && f.isComp==constant(true).isComp) {
				return NOT( g );
			}
			if (g.ind==constant(true).ind && g.isComp==constant(true).isComp) {
				return NOT( f );
			}
			if (f.ind==NOT(g).ind && f.isComp==NOT(g).isComp){
				return constant( true );
			}
			if (NOT(f).ind==g.ind && NOT(f).isComp==g.isComp){
				return constant( true );
			}


			Node const& F = nodes[f.ind];
			Node const& G = nodes[g.ind];
			var_t x;
			C_Edge f0, f1, g0, g1;
			//in the following lines we transpose the complementation of f into f0,f1
			if ( F.v < G.v ) /* F is on top of G */
			{	
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) /* G is on top of F */
			{	
				x = G.v;
				f0 = f1 = f;
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}
			else /* F and G are at the same level */
			{	
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}
			
			C_Edge const r0 = XOR( f0, g0 );
			C_Edge const r1 = XOR( f1, g1 );
			//deref(f1);
    		//deref(g1);			
			computed_table_XOR.emplace_back( std:: make_pair(std:: make_tuple(f.ind, f.isComp, g.ind, g.isComp), unique( x, r1, r0 )) );
			return (unique( x, r1, r0 ));
		}


		/* Compute f & g */
		C_Edge AND( C_Edge f, C_Edge g ) 
		{		  
			//assert( f < nodes.size() && "Make sure f exists." );
			//assert( g < nodes.size() && "Make sure g exists." );
			++num_invoke_and;

			//if (f.ind < g.ind){std::swap(f,g);}

			// computed table lookup:
			for (int i = 0; i < computed_table_AND.size(); i++)
			{
				//AND(f,g)
				if (computed_table_AND[i].first  == std::make_tuple(f.ind, f.isComp, g.ind, g.isComp))
				{
					return  computed_table_AND[i].second;
				}
			}

			/* trivial cases */
			if ( (f.ind==constant(false).ind && f.isComp==constant(false).isComp) || (g.ind==constant(false).ind && g.isComp==constant(false).isComp) ) 
			{
				return constant( false );
			}
			if (f.ind==constant(true).ind && f.isComp==constant(true).isComp)
			{
				return g;
			}
			if (g.ind==constant(true).ind && g.isComp==constant(true).isComp) 
			{
				return f;
			}
			if (f.ind==g.ind && f.isComp==g.isComp) 
			{
				return f;
			}

			Node const& F = nodes[f.ind];
			Node const& G = nodes[g.ind];
			var_t x;
			
			C_Edge f0, f1, g0, g1;
			if ( F.v < G.v ) { /* F is on top of G */
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) { /* G is on top of F */
				
				x = G.v;
				f0 = f1 = f;
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}
			else { /* F and G are at the same level */
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}

			C_Edge const r0 = AND( f0, g0 );
			C_Edge const r1 = AND( f1, g1 );
			//deref(f1);
    		//deref(g1);
			computed_table_AND.emplace_back( std:: make_pair(std:: make_tuple(f.ind, f.isComp, g.ind, g.isComp), unique( x, r1, r0 )) );
			return (unique( x, r1, r0 ));
		}

		/* Compute f | g */
		C_Edge OR( C_Edge f, C_Edge g ) 
		{
			//assert( f < nodes.size() && "Make sure f exists." );
			//assert( g < nodes.size() && "Make sure g exists." );
			++num_invoke_or;
			
			for (int i = 0; i < computed_table_OR.size(); i++)
			{
				//OR(f,g)
				if (computed_table_OR[i].first  == std::make_tuple(f.ind, f.isComp, g.ind, g.isComp))
				{
					return  computed_table_OR[i].second;
				}
			}


			/* trivial cases */
      		if ( (f.ind==constant(true).ind && f.isComp==constant(true).isComp) || (g.ind==constant(true).ind && g.isComp==constant(true).isComp) ) 
			{
				return constant( true );
			}
			if (f.ind==constant(false).ind && f.isComp==constant(false).isComp) 
			{
				return g;
			}
			if (g.ind==constant(false).ind && g.isComp==constant(false).isComp) 
			{
				return f;
			}
			if (f.ind==g.ind && f.isComp==g.isComp) 
			{
				return f;
			}

			Node const& F = nodes[f.ind];
			Node const& G = nodes[g.ind];
			var_t x;
			C_Edge f0, f1, g0, g1;
			if ( F.v < G.v ) /* F is on top of G */
			{
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) /* G is on top of F */
			{
				x = G.v;
				f0 = f1 = f;
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}
			else /* F and G are at the same level */
			{	
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
				g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
			}

			C_Edge const r0 = OR( f0, g0 );
			C_Edge const r1 = OR( f1, g1 );
			
			//deref(f1);
    		//deref(g1);
    		//deref(h1);
			computed_table_OR.emplace_back( std:: make_pair(std:: make_tuple(f.ind, f.isComp, g.ind, g.isComp), unique( x, r1, r0 )) );
			return (unique( x, r1, r0 ));
		}

		/* Compute ITE(f, g, h), i.e., f ? g : h */
		C_Edge ITE( C_Edge f, C_Edge g, C_Edge h ) 
		{
			//assert( f < nodes.size() && "Make sure f exists." );
			//assert( g < nodes.size() && "Make sure g exists." );
			//assert( h < nodes.size() && "Make sure h exists." );
			
			++num_invoke_ite;

			for (int i = 0; i < computed_table_ITE.size(); i++)
			{
				//ITE(f,g,h)
				if (computed_table_ITE[i].first  == std::make_tuple(f.ind, f.isComp, g.ind, g.isComp, h.ind, h.isComp))
				{
					return  computed_table_ITE[i].second;
				}
				//ITE(f',h,g)
				else if(computed_table_ITE[i].first  == std::make_tuple(f.ind, !f.isComp, h.ind, h.isComp, g.ind, g.isComp))
				{
					return  computed_table_ITE[i].second;
				}
			}

			/* trivial cases */
			if (f.ind==constant(true).ind && f.isComp==constant(true).isComp) 
			{
				return g;
			}
			if (f.ind==constant(false).ind && f.isComp==constant(false).isComp) 
			{
				return h;
			}
			if (g.ind==h.ind && g.isComp==h.isComp) 
			{
				return g;
			}
	   
		

			Node const& F = nodes[f.ind];
			Node const& G = nodes[g.ind];
			Node const& H = nodes[h.ind];
			var_t x;
			C_Edge f0, f1, g0, g1, h0, h1;
			if ( F.v <= G.v && F.v <= H.v ) { /* F is not lower than both G and H */
				x = F.v;
				f0 = !f.isComp ? F.E : C_Edge {F.E.ind, ! F.E.isComp};
				f1 = !f.isComp ? F.T : C_Edge {F.T.ind, ! F.T.isComp};
				if ( G.v == F.v ) {
					g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
					g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
				}
				else {
					g0 = g1 = g;
				}
				if ( H.v == F.v ) {
					h0 = !h.isComp ? H.E : C_Edge {H.E.ind, ! H.E.isComp};
					h1 = !h.isComp ? H.T : C_Edge {H.T.ind, ! H.T.isComp};
				}
				else {
					h0 = h1 = h;
				}
			}
			else { /* F.v > min(G.v, H.v) */
				f0 = f1 = f;
				if ( G.v < H.v ) {
					x = G.v;
					g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
					g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
					h0 = h1 = h;
				}
				else if ( H.v < G.v ) {
					x = H.v;
					g0 = g1 = g;
					h0 = !h.isComp ? H.E : C_Edge {H.E.ind, ! H.E.isComp};
					h1 = !h.isComp ? H.T : C_Edge {H.T.ind, ! H.T.isComp};
				}
				else { /* G.v == H.v */ 
					x = G.v;
					g0 = !g.isComp ? G.E : C_Edge {G.E.ind, ! G.E.isComp};
					g1 = !g.isComp ? G.T : C_Edge {G.T.ind, ! G.T.isComp};
					h0 = !h.isComp ? H.E : C_Edge {H.E.ind, ! H.E.isComp};
					h1 = !h.isComp ? H.T : C_Edge {H.T.ind, ! H.T.isComp};
				}
			}
			C_Edge const r0 = ITE( f0, g0, h0 );
			C_Edge const r1 = ITE( f1, g1, h1 );
			//deref(f1);
    		//deref(g1);
    		//deref(h1);
			computed_table_ITE.emplace_back( std:: make_pair(std:: make_tuple(f.ind, f.isComp, g.ind, g.isComp, h.ind, h.isComp), unique( x, r1, r0 )) );
			return (unique( x, r1, r0 ));
		}

		/**********************************************************/
		/***************** Printing and Evaluating ****************/
		/**********************************************************/

		/* Print the BDD rooted at node `f`. */
		// a few adaptation due to the fact that f is now a C_Edge
		void print( C_Edge f, std::ostream& os = std::cout ) const{
			for ( auto i = 0u; i < nodes[f.ind].v; ++i ){
				os << "  ";
			}
			if ( f.ind <= 0 ){
				os << "node " << f.ind << ": constant " << f.isComp << std::endl;
			}
			else{
				os << "node " << f.ind << ": var = " << nodes[f.ind].v << ", T = " << nodes[f.ind].T.ind
				<< ", E = " << nodes[f.ind].E.ind << std::endl;
				for ( auto i = 0u; i < nodes[f.ind].v; ++i ){
					os << "  ";
				}
				os << "> THEN branch" << std::endl;
				print( nodes[f.ind].T, os );
				for ( auto i = 0u; i < nodes[f.ind].v; ++i ){
					os << "  ";
				}
				os << "> ELSE branch" << std::endl;
				print( nodes[f.ind].E, os );
			}
		}
			
		
		
		/* Get the truth table of the BDD rooted at node f. */
		Truth_Table get_tt( C_Edge f ) {
			//assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );
			//assert( f < nodes.size() && "Make sure f exists." );
			
			if (f.ind==constant(false).ind && f.isComp==constant(false).isComp){
				return Truth_Table( num_vars() );
			}
			else if (f.ind==constant(true).ind && f.isComp==constant(true).isComp){
				return ~Truth_Table( num_vars() );
			}
			/* Shannon expansion: f = x f_x + x' f_x' */
			var_t const x = nodes[f.ind].v;
			C_Edge const fx = C_Edge({nodes[f.ind].T.ind, false});
    		C_Edge const fnx = C_Edge({nodes[f.ind].E.ind, false});			
			Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
			Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );

			bool EComp = nodes[f.ind].E.isComp ;
			
			if (f.isComp and EComp ) 
			{
				return ~( ( tt_x & get_tt(fx) ) | ( ~ tt_x & (~ get_tt(fnx) ) ) );
			}
			else if (!f.isComp and !EComp ) 
			{
				return ( tt_x & get_tt(fx) ) | ( ~ tt_x & get_tt(fnx) );
			}
			else if (f.isComp and !EComp ) 
			{
				return ~( ( tt_x & get_tt(fx) ) | ( ~ tt_x & get_tt(fnx) ) );
			}
			else if (!f.isComp and EComp ) 
			{
				return ( tt_x & get_tt(fx) ) |  ( ~ tt_x & (~ get_tt(fnx) ) );
			}
		}

		/* Whether `f` is dead (having a reference count of 0). */
		bool is_dead( index_t f ) const
		{			
			return ! nodes[f].isAlive;
		}

		/* Get the number of living nodes in the whole package, excluding constants. */
		uint64_t num_nodes() const
		{
			uint64_t n = 0u;
			for ( auto i = 0; i < nodes.size(); ++i )
			{
				if ( !is_dead( i ) )
				{
					++n;
				}
			}
			return n;
		}

	  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
		uint64_t num_nodes( C_Edge f ){
			assert( f.ind < nodes.size() && "Make sure f exists." );

			if ( (f.ind==constant(false).ind && f.isComp==constant(false).isComp) || (f.ind==constant(true).ind && f.isComp==constant(true).isComp) )
			{
				return 0u;
			}

			std::vector<bool> visited( nodes.size(), false );
			visited[0] = true;
			//visited[1] = true;

			return num_nodes_rec( f, visited );
		}

		uint64_t num_invoke() const {
			return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
		}

	private:
		/**********************************************************/
		/******************** Helper Functions ********************/
		/**********************************************************/

		uint64_t num_nodes_rec( C_Edge f, std::vector<bool>& visited ) const 
		{
			//assert( f < nodes.size() && "Make sure f exists." );

			uint64_t n = 0u;
			Node const& F = nodes[f.ind];
			//assert( F.T < nodes.size() && "Make sure the children exist." );
			//assert( F.E < nodes.size() && "Make sure the children exist." );
			if ( !visited[F.T.ind] ) {
				n += num_nodes_rec( F.T, visited );
				visited[F.T.ind] = true;
			}
			if ( !visited[F.E.ind] ) {
				n += num_nodes_rec( F.E, visited );
				visited[F.E.ind] = true;
			}
			return n + 1u;
		}

	private:
		std::vector<Node> nodes;
		
		std::vector<std::unordered_map<std::tuple<index_t, index_t, bool>, C_Edge>> unique_table;
		/* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
		 * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
		 * See the implementation of `unique` for example usage. */

		/* statistics */
		uint64_t num_invoke_not, num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;

		std::vector <std::pair < std::tuple <index_t, bool, index_t, bool, index_t, bool>, C_Edge > > computed_table_ITE;
  		std::vector <std::pair < std::tuple <index_t, bool, index_t, bool >, C_Edge > > computed_table_OR;
  		std::vector <std::pair < std::tuple <index_t, bool, index_t, bool >, C_Edge > > computed_table_AND;
  		std::vector <std::pair < std::tuple <index_t, bool, index_t, bool >, C_Edge > > computed_table_XOR;
};
