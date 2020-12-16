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
	inline void hash_combine( size_t& seed, T const& v ) {
		seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<pair<uint32_t, uint32_t>> {
		using argument_type = pair<uint32_t, uint32_t>;
		using result_type = size_t;
		result_type operator() ( argument_type const& in ) const {
			result_type seed = 0;
			hash_combine( seed, in.first );
			hash_combine( seed, in.second );
			return seed;
		}
	};


	template<>
	struct hash<tuple<uint32_t, uint32_t, bool>> { //for unique table to know if the E edge is complemented on this node
			using argument_type = tuple<uint32_t, uint32_t, bool>;
			using result_type = size_t;
			result_type operator() ( argument_type const& in ) const {
				result_type seed = 0;
				hash_combine( seed, std::get<0>(in) );
				hash_combine( seed, std::get<1>(in) );
				hash_combine( seed, std::get<2>(in) );
				return seed;
			}
		};

	template<>
	struct hash<tuple<uint32_t, bool, uint32_t, bool, uint32_t>> { // for computed table ITE
		using argument_type = tuple<uint32_t, bool, uint32_t, bool, uint32_t>;
		using result_type = size_t;
		result_type operator() ( argument_type const& in ) const {
			result_type seed = 0;
			hash_combine( seed, std::get<0>(in) );
			hash_combine( seed, std::get<1>(in) );
			hash_combine( seed, std::get<2>(in) );
			hash_combine( seed, std::get<3>(in) );
			hash_combine( seed, std::get<4>(in) );
			return seed;
		}
	};
	
	template<>
	struct hash<tuple<uint32_t, bool, uint32_t, bool, uint32_t, bool>> { // for computed table ITE
		using argument_type = tuple<uint32_t, bool, uint32_t, bool, uint32_t, bool>;
		using result_type = size_t;
		result_type operator() ( argument_type const& in ) const {
			result_type seed = 0;
			hash_combine( seed, std::get<0>(in) );
			hash_combine( seed, std::get<1>(in) );
			hash_combine( seed, std::get<2>(in) );
			hash_combine( seed, std::get<3>(in) );
			hash_combine( seed, std::get<4>(in) );
			hash_combine( seed, std::get<5>(in) );
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
		 
		using index_edge_t = uint32_t;
		/* Declaring `index_edge_t` as an alias for an unsigned integer.
		 * This is just for easier understanding of the code.
		 * This datatype will be used for edges indices. */

		using var_t = uint32_t;
		/* Similarly, declare `var_t` also as an alias for an unsigned integer.
		 * This datatype will be used for representing variables. */
		
		

		
	private:

		struct edge {   																																// new structure edge, used to point on a node and to know if it is complemented
			uint32_t node_index;
			bool complemented;
		};
	 
		struct Node { // unchanged Node structure
			var_t v; /* corresponding variable */
			edge T; /* index of pointing node of  THEN child */
			edge E; /* index of pointing node of ELSE child */
			bool alive;
		};

	public:
		explicit BDD( uint32_t num_vars ) : 
			unique_table( num_vars ), // the list of already build node
			num_invoke_and( 0u ), // number of time we've called AND
			num_invoke_or( 0u ),  // number of time we've called OR
			num_invoke_xor( 0u ), // number of time we've called XOR
			num_invoke_ite( 0u ) {  // number of time we've called ITE
				nodes.emplace_back( Node({num_vars, 0, 0, true}) ); /* constant 1 */ 																				//this is the only constant needed for complemented edge graphs
		
				/* `nodes` is initialized with 1 `Node`s representing the terminal (positive) nodes.
				 * `v` is `num_vars` and his indice is 0.
				 * (Note that the real variables range from 0 to `num_vars - 1`.)
				 * Both of their children point to themselves, just for convenient representation.
				 *
				 * `unique_table` is initialized with `num_vars` empty maps. */
			}

		/**********************************************************/
		/***************** Basic Building Blocks ******************/
		/**********************************************************/

	  
	  

		uint32_t num_vars() const {
			return unique_table.size();
		}

		/* Get the (index of) constant node. */
		edge constant( bool value ) const {
			/* for complemented edge gaph we only return a edge pointing the constant 1, which is complemented if is supposed to point 0*/
			return ( edge { 0, !value } ) ;
		}
	  
		inline bool IS_EQUAL_EDGE(edge f, edge g){ 																										//a simple test to know if f and g are equal : same pointing node and both complemented of not
			return (f.node_index == g.node_index and f.complemented == g.complemented);
		}

		/* Look up (if exist) or build (if not) the node with variable `var`,
		 * THEN child `T`, and ELSE child `E`. */
	   
		edge unique( var_t var, edge T, edge E ) {
			//std::cout << " Entring unique with " << T.node_index << " " << T.complemented << " and "  << E.node_index << " " << E.complemented << std::endl;
			assert( var < num_vars() && "Variables range from 0 to `num_vars - 1`." ); 																	//checking that this is a value 
			assert( T.node_index < nodes.size() && "Make sure the children exist." ); 																	//checking that the pointing node eists
			assert( E.node_index < nodes.size() && "Make sure the children exist." );  																	//checking that the pointing node eists
			assert( nodes[T.node_index].v > var && "With static variable order, children can only be below the node." ); 								//checking variable order
			assert( nodes[E.node_index].v > var && "With static variable order, children can only be below the node." ); 								//checking variable order
		
			/* first we need to reset things in order to delete complement on T edge */
			bool node_complemented = T.complemented ; 																									//we complement the node if the then edge is complemented as T edge shoud not be complemented
			if (T.complemented){
				T.complemented = false; 																												/* THEN complemented edge not allowed*/
				E.complemented = !E.complemented; 																										/* to balance the fact that we will complement the node */
			}
		
			/* At this point we are allowed to proced as usual : we just have to check if the node already exists and return an complemented edge on not*/
		
			/* Reduction rule: Identical children : need to have the same children but also, E must not be complemented thanks to before, T can't be complemented */
			if ( (T.node_index == E.node_index) and not(E.complemented)){
				//std::cout << " Exiting unique with " << T.node_index << " " << node_complemented << " by reduction rule " << std::endl;
				return ( edge { T.node_index, node_complemented } ); 																					//we simply return the edge to this existing node, complemented if the node has to be
			}
		
			/* if we arrive there, we have to look in computed. We do not take care of wather or not the node is complemented as the edge, that we will return, will take care of that*/
			/*we have to look in computed table to know if the node we want is there*/

			/* Look up in the unique table. */
			const auto it = unique_table[var].find( std::make_tuple(T.node_index, E.node_index, E.complemented) ); 										//a pointer in unique table, on the node or on the end of the hash table
			if ( it != unique_table[var].end() ) { //if not end(), the existing node already exists 
				/* The required node already exists. Return it, we just have to complement ti wether or not it has to be*/
				//std::cout << " Exiting unique with " << it->second.node_index << " " << bool(it->second.complemented != node_complemented) << " by unique table " << std::endl;
				return ( edge { it->second.node_index, node_complemented } );
			}

			//else 																																		//will pass there in other cases
		
			/* Create a new node and insert it to the unique table. */
			index_t new_index;
			if (table_empty_node.size()== 0){ // we have no empty nodes for the moment
				new_index = nodes.size();																// we give an index to the new node
				nodes.emplace_back( Node({var, T, E, true}) );
			}
			else {
				new_index = table_empty_node.at(0);
				table_empty_node.erase (table_empty_node.begin()); //we now take care of this node !!!
				nodes.at(new_index)= Node({var, T, E, true});
				nodes.at(new_index).alive=true;
			}																																			// we add it in the vectors of nodes
			unique_table[var][std::make_tuple(T.node_index, E.node_index, E.complemented)] = edge { new_index, node_complemented } ; 					//we add it in computed table
			//std::cout << "The required node do not exists in unique"  << std::endl;
			//std::cout << " Exiting unique with " << new_index << " " << node_complemented << " by computation " << std::endl;
			return ( edge { new_index, node_complemented } ); 																							// and we return it
	   
		}

		/* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
		edge literal( var_t var, bool complement = false ) {																							// used to generate the variables
			return unique( var, constant( !complement ), constant( complement ) );																		// will create a node pointing to the differents constants
		}
		
		
		/**********************************************************/
		/*********************** Ref & Deref **********************/
		/**********************************************************/
		
		std::vector<index_t> add_child (index_t node, std::vector<index_t> reachable_nodes){
			if (node != 0) {
				/*terminal case, we have reach the bottom of the tree */
				bool find = false;
				for (uint32_t i = 0; i < reachable_nodes.size(); i++){
					if (reachable_nodes.at(i) == node){
						/* if we already had a look to this node, form this node, no need to add it a other time */
						find = true;
						break;
					}
				}
				if (not find){ /* if this is the first time that we see this node, need to add it and its child*/
					reachable_nodes.emplace_back(node);
					
					reachable_nodes = add_child (nodes[node].T.node_index, reachable_nodes);
					reachable_nodes = add_child (nodes[node].E.node_index, reachable_nodes);
				}
			}
			return reachable_nodes;
		}
		
		
		edge ref( edge f ) {
			std::vector<index_t> reachable_nodes;
			/* then we register all the child of this node thank to our recusive function*/
			reachable_nodes = add_child(f.node_index, reachable_nodes);
			reachable_nodes.emplace_back(0); //because it is true, allows us not to manage empty vectors
			ref_child.emplace_back(reachable_nodes);
			/* TODO */
			return f;
		}

		void deref( edge f ) {
			/* We will look to all the child of f and dereference them if they are not rechable by an other reachable node */
			/* if a node is dereferenced, for the moment, we just put his member .alive to false */
			
			/*first we need to find the list of child node of f*/
			uint32_t index_f;
			bool finded = false; 
			for (uint32_t i = 0; i < ref_child.size(); i++){
				if (ref_child.at(i).at(0) == f.node_index) {
					// this is the node we want to dereference
					finded = true;
					index_f = i;
					break;
				}
			}
			
			assert( finded == true && "Make sure that f as been referenced." );
			
			//we first save f and its child in a new vector
			std::vector<index_t> reachable_nodes_from_f = ref_child.at(index_f);
			
			//we can delete f from the vector referenced 
			ref_child.erase( ref_child.begin() + index_f );
			
			// then we can test if each node f or child of f is used in an other referenced node
			for (uint32_t i = 0; i < reachable_nodes_from_f.size(); i++){
				bool exists_somewhere_else = false;
				for (uint32_t j = 0; j < ref_child.size(); j++){
					for (uint32_t k = 0; k < ref_child.at(j).size(); k++){
						if (ref_child.at(j).at(k) == reachable_nodes_from_f.at(i)){
							exists_somewhere_else = true;
						}
					}
				}
				if (not exists_somewhere_else) {
					//this node is only rechable from f, so we can kill it #unpeudeviolancedanscemonde
					nodes[reachable_nodes_from_f.at(i)].alive=false;
					table_empty_node.emplace_back(reachable_nodes_from_f.at(i));
				}
			}
		}
	  
	  
	  
		/**********************************************************/
		/********************* BDD Operations *********************/
		/**********************************************************/

		/* Compute ~f */
		edge NOT( edge f ) {
			//std::cout << "NOT " << f.node_index << " " << f.complemented << std::endl;
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			return  ( edge { f.node_index, !f.complemented } ); 																						//here not a problem
		}

		/* Compute f ^ g */
		edge XOR( edge f, edge g ) {
			++num_invoke_xor;
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			assert( g.node_index < nodes.size() && "Make sure g exists." );
			
			if (f.node_index < g.node_index){
				edge buffer = f;
				f = g;
				g = buffer ;
			}
			
			// test if already computed
			//std::cout << "XOR " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented  << std::endl;
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it = computed_table_2op.find( std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 1) );
			if ( it != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " XOR computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
				return (it->second); //we will have to complement the result only if only one of f or g is complemented so we use a XOR between the member describing that //
			}
			
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it2 = computed_table_2op.find( std::make_tuple(f.node_index, !f.complemented, g.node_index, !g.complemented, 1) );
			if ( it2 != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " XOR computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
				return (it2->second); //we will have to complement the result only if only one of f or g is complemented so we use a XOR between the member describing that //
			}
			
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it3 = computed_table_2op.find( std::make_tuple(f.node_index, f.complemented, g.node_index, !g.complemented, 1) );
			if ( it3 != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " XOR computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
				return (edge{it3->second.node_index, !it3->second.complemented}); //we will have to complement the result only if only one of f or g is complemented so we use a XOR between the member describing that //
			}
			
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it4 = computed_table_2op.find( std::make_tuple(f.node_index, !f.complemented, g.node_index, g.complemented, 1) );
			if ( it4 != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " XOR computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
				return (edge{it4->second.node_index, !it4->second.complemented}); //we will have to complement the result only if only one of f or g is complemented so we use a XOR between the member describing that //
			}

			/* trivial cases */
			if ( IS_EQUAL_EDGE(f, g ) ) {
				//std::cout << "trivial case : f = g return " << "0" << " " << "1"  << std::endl;
				return constant( false );
			}
			if ( IS_EQUAL_EDGE( f, constant( false ) ) ) {
				//std::cout << "trivial case : f  false return " << g.node_index << " " << g.complemented  << std::endl;
				return g;
			}
			if ( IS_EQUAL_EDGE( g, constant( false ) ) ) {
				//std::cout << "trivial case : g false return " << f.node_index << " " << f.complemented  << std::endl;
				return f;
			}
			if ( IS_EQUAL_EDGE ( f, constant( true ) ) ) {
				//std::cout << "trivial case : f true return " << NOT(g).node_index << " " << NOT(g).complemented  << std::endl;
				return NOT( g );
			}
			if ( IS_EQUAL_EDGE ( g, constant( true ) ) ) {
				//std::cout << "trivial case : g true return " << NOT(f).node_index << " " << NOT(f).complemented  << std::endl;
				return NOT( f );
			}
			if ( IS_EQUAL_EDGE(f, NOT( g ) ) ){
				//std::cout << "trivial case : f = g' return " << "0" << " " << "0"  << std::endl;
				return constant( true );
			}
			if ( IS_EQUAL_EDGE(NOT(f), g ) ){
				//std::cout << "trivial case : f = g' return " << "0" << " " << "0"  << std::endl;
				return constant( true );
			}


			Node const& F = nodes[f.node_index];
			Node const& G = nodes[g.node_index];
			var_t x;
			edge f0, f1, g0, g1;
			if ( F.v < G.v ) { /* F is on top of G */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, false};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) { /* G is on top of F */
				x = G.v;
				f0 = f1 = f;
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, false};
			}
			else { /* F and G are at the same level */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, false};
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, false};
			}
			
			edge const r0 = XOR( f0, g0 );
			edge const r1 = XOR( f1, g1 );
			edge r = unique( x, r1, r0 );
			//add to computed

			computed_table_2op.emplace(std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 1) ,r);

			//std::cout << " XOR call end " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << r.node_index << " " << r.complemented << std::endl;
			return r;
		}


		/* Compute f & g */
		edge AND( edge f, edge g ) {
			++num_invoke_and;
		  
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			assert( g.node_index < nodes.size() && "Make sure g exists." );
		
			//std::cout << "AND " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented  << std::endl;
		
			// test if already computed
		
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it = computed_table_2op.find( std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 2) );
			if ( it != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << it->second  << std::endl;
				//std::cout << " AND computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
		  
				return it->second;
			}

			/* trivial cases */
			if ( IS_EQUAL_EDGE(f, constant( false ) ) || IS_EQUAL_EDGE( g, constant( false ) ) ) {
				//std::cout << "trivial case : f or g false return " << "0" << " " << "1"  << std::endl;
				return constant( false );
			}
			if ( IS_EQUAL_EDGE ( f,constant( true ) ) ) {
				//std::cout << "trivial case : f is true return " << g.node_index << " " << g.complemented  << std::endl;
				return g;
			}
			if ( IS_EQUAL_EDGE ( g, constant( true ) ) ) {
				//std::cout << "trivial case : g is true return " << f.node_index << " " << f.complemented  << std::endl;
				return f;
			}
			if ( IS_EQUAL_EDGE ( f, g ) ) {
				//std::cout << "trivial case : f = g return " << f.node_index << " " << f.complemented  << std::endl;
				return f;
			}

			Node const& F = nodes[f.node_index];
			Node const& G = nodes[g.node_index];
			var_t x;
			
			edge f0, f1, g0, g1;
			if ( F.v < G.v ) { /* F is on top of G */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, true};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) { /* G is on top of F */
				
				x = G.v;
				f0 = f1 = f;
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, true};
			}
			else { /* F and G are at the same level */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, true};
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, true};
			}
			//std::cout << "var : " << x << std::endl;
			edge const r0 = AND( f0, g0 );
			edge const r1 = AND( f1, g1 );
			edge r = unique( x, r1, r0 );
			//add to computed
			computed_table_2op.emplace(std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 2) ,r);
			//std::cout << " AND call end " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << r.node_index << " " << r.complemented << std::endl;
			return r;
		}

		/* Compute f | g */
		edge OR( edge f, edge g ) {
			
			++num_invoke_or;
			
			//std::cout << "OR " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented  << std::endl;
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			assert( g.node_index < nodes.size() && "Make sure g exists." );
		
			// test if already computed
		
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge>::const_iterator it = computed_table_2op.find( std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 3) );
			if ( it != computed_table_2op.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " OR computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << it->second.node_index << " " << it->second.complemented << std::endl;
				return it->second;
			}
		
		
			
		
			//std::cout << " OR " << std::endl;

			/* trivial cases */
			if ( IS_EQUAL_EDGE (f, constant( true ) ) || IS_EQUAL_EDGE (g, constant( true ) )) {
				//std::cout << "trivial case : f or g True return " << "0" << " " << "0"  << std::endl;
				return constant( true );
			}
			if ( IS_EQUAL_EDGE (f, constant( false ) )) {
				//std::cout << "trivial case : f is false return " << g.node_index << " " << g.complemented  << std::endl;
				return g;
			}
			if ( IS_EQUAL_EDGE (g, constant( false ) )) {
				//std::cout << "trivial case : g is false return " << f.node_index << " " << f.complemented  << std::endl;
				return f;
			}
			if ( IS_EQUAL_EDGE (f, g )) {
				//std::cout << "trivial case : f = g return " << f.node_index << " " << f.complemented  << std::endl;
				return f;
			}

			Node const& F = nodes[f.node_index];
			Node const& G = nodes[g.node_index];
			var_t x;
			edge f0, f1, g0, g1;
			if ( F.v < G.v ) { /* F is on top of G */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, false};
				g0 = g1 = g;
			}
			else if ( G.v < F.v ) { /* G is on top of F */
				x = G.v;
				f0 = f1 = f;
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, false};
			}
			else { /* F and G are at the same level */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, false};
				g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
				g1 = !g.complemented ? G.T : edge {G.T.node_index, false};
			}

			edge const r0 = OR( f0, g0 );
			edge const r1 = OR( f1, g1 );
			edge r = unique( x, r1, r0 );
			//add to computed
			computed_table_2op.emplace(std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, 3) ,r);
			//std::cout << " OR call end " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " returned " << r.node_index << " " << r.complemented << std::endl;
			return r;
		}

		/* Compute ITE(f, g, h), i.e., f ? g : h */
		edge ITE( edge f, edge g, edge h ) {
			++num_invoke_ite;
			
			//return (OR (AND(f,g), AND(NOT(f),h)));
			
						
			//std::cout << "ITE " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented  << " " << h.node_index << " " << h.complemented << std::endl;
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			assert( g.node_index < nodes.size() && "Make sure g exists." );
			assert( h.node_index < nodes.size() && "Make sure h exists." );
			
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t, bool>, edge>::const_iterator it = computed_table_ITE.find( std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, h.node_index, h.complemented) );
			if ( it != computed_table_ITE.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " ITE computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " " << h.node_index << " " << h.complemented << " returned " << " " << it->second.node_index << " " << it->second.complemented << std::endl;
				return it->second;
			}
			const std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t, bool>, edge>::const_iterator it2 = computed_table_ITE.find( std::make_tuple(f.node_index, !f.complemented, h.node_index, h.complemented, g.node_index, g.complemented) );
			if ( it2 != computed_table_ITE.end() ) {
				/* The required node already exists. Return it. */
				//std::cout << " ITE computed " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " " << h.node_index << " " << h.complemented << " returned " << " " << it2->second.node_index << " " << it2->second.complemented << std::endl;
				return it2->second;
			}

			/* trivial cases */
			if ( IS_EQUAL_EDGE(f, constant( true ) ) ) {
				//std::cout << "trivial case : f is true return g " << g.node_index << " " << g.complemented  << std::endl;
				return g;
			}
			if ( IS_EQUAL_EDGE(f, constant( false ) ) ) {
				//std::cout << "trivial case : f is false return h " << h.node_index << " " << h.complemented  << std::endl;
				return h;
			}
			if ( IS_EQUAL_EDGE(g, h ) ) {
				//std::cout << "trivial case : g = h return " << g.node_index << " " << g.complemented  << std::endl;
				return g;
			}
	   
		

			Node const& F = nodes[f.node_index];
			Node const& G = nodes[g.node_index];
			Node const& H = nodes[h.node_index];
			var_t x;
			edge f0, f1, g0, g1, h0, h1;
			if ( F.v <= G.v && F.v <= H.v ) { /* F is not lower than both G and H */
				x = F.v;
				f0 = !f.complemented ? F.E : edge {F.E.node_index, ! F.E.complemented}; //if f is complmented, we exchange the members
				f1 = !f.complemented ? F.T : edge {F.T.node_index, true};
				if ( G.v == F.v ) {
					g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
					g1 = !g.complemented ? G.T : edge {G.T.node_index, true};
				}
				else {
					g0 = g1 = g;
				}
				if ( H.v == F.v ) {
					h0 = !h.complemented ? H.E : edge {H.E.node_index, ! H.E.complemented}; //if f is complmented, we exchange the members
					h1 = !h.complemented ? H.T : edge {H.T.node_index, true};
				}
				else {
					h0 = h1 = h;
				}
			}
			else { /* F.v > min(G.v, H.v) */
				f0 = f1 = f;
				if ( G.v < H.v ) {
					x = G.v;
					g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
					g1 = !g.complemented ? G.T : edge {G.T.node_index, true};
					h0 = h1 = h;
				}
				else if ( H.v < G.v ) {
					x = H.v;
					g0 = g1 = g;
					h0 = !h.complemented ? H.E : edge {H.E.node_index, ! H.E.complemented}; //if f is complmented, we exchange the members
					h1 = !h.complemented ? H.T : edge {H.T.node_index, true};
				}
				else { /* G.v == H.v */ 
					x = G.v;
					g0 = !g.complemented ? G.E : edge {G.E.node_index, ! G.E.complemented}; //if f is complmented, we exchange the members
					g1 = !g.complemented ? G.T : edge {G.T.node_index, true};
					h0 = !h.complemented ? H.E : edge {H.E.node_index, ! H.E.complemented}; //if f is complmented, we exchange the members
					h1 = !h.complemented ? H.T : edge {H.T.node_index, true};
				}
			}

			edge const r0 = ITE( f0, g0, h0 );
			//std::cout << " the ITE call " << f0.node_index << " " << f0.complemented << " " << g0.node_index << " " << g0.complemented << " " << h0.node_index << " " << h0.complemented << " returned " << " " << r0.node_index << " " << r0.complemented << std::endl;
			edge const r1 = ITE( f1, g1, h1 );
			//std::cout << " the ITE call " << f1.node_index << " " << f1.complemented << " " << g1.node_index << " " << g1.complemented << " " << h1.node_index << " " << h1.complemented << " returned " << " " << r1.node_index << " " << r1.complemented << std::endl;
			edge r = unique( x, r1, r0 );
			//add to computed
			computed_table_ITE.emplace(std::make_tuple(f.node_index, f.complemented, g.node_index, g.complemented, h.node_index, h.complemented) ,r);
			//std::cout << " ITE call end " << f.node_index << " " << f.complemented << " " << g.node_index << " " << g.complemented << " " << h.node_index << " " << h.complemented << " returned " << " " << r.node_index << " " << r.complemented << std::endl;
			return r;
		}

		/**********************************************************/
		/***************** Printing and Evaluating ****************/
		/**********************************************************/

		/* Print the BDD rooted at node `f`. */
		void print( edge f, std::ostream& os = std::cout ) const{
			for ( auto i = 0u; i < nodes[f.node_index].v; ++i ){
				os << "  ";
			}
			if ( f.node_index <= 0 ){
				os << "node " << f.node_index << ": constant " << f.complemented << std::endl;
			}
			else{
				os << "node " << f.node_index << ": var = " << nodes[f.node_index].v << ", T = " << nodes[f.node_index].T.node_index
				<< ", E = " << nodes[f.node_index].E.node_index << std::endl;
				for ( auto i = 0u; i < nodes[f.node_index].v; ++i ){
					os << "  ";
				}
				os << "> THEN branch" << std::endl;
				print( nodes[f.node_index].T, os );
				for ( auto i = 0u; i < nodes[f.node_index].v; ++i ){
					os << "  ";
				}
				os << "> ELSE branch" << std::endl;
				print( nodes[f.node_index].E, os );
			}
		}
		
		void get_tree(edge f){
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			for (uint32_t i = 1 ; i < nodes.size() ; i++){
				std::cout << "Node " << i << " vaariable : " << nodes[i].v << std::endl;
				std::cout << "Node " << i << " Then edge : " << nodes[i].T.node_index << " " << nodes[i].T.complemented << std::endl;
				std::cout << "Node " << i << " Else edge : " << nodes[i].E.node_index << " " << nodes[i].E.complemented << std::endl;
				std::cout << "Node " << i << " alive : " << nodes[i].alive  << std::endl;
				std::cout << std::endl;
			}

		}
			
		
		
		/* Get the truth table of the BDD rooted at node f. */
		Truth_Table get_tt( edge f ) {
			//std::cout << "entring get_tt with :  " << f.node_index <<" and " << f.complemented << std::endl;
			assert( f.node_index < nodes.size() && "Make sure f exists." );
			//assert( num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables." );

			if ( IS_EQUAL_EDGE(f, constant( false ) ) ){
				//std::cout << "f constant false  " << std::endl;
				return Truth_Table( num_vars() );
			}
			else if ( IS_EQUAL_EDGE( f, constant( true ) ) ){
				//std::cout << "f constant true  " << std::endl;
				return ~Truth_Table( num_vars() );
			}
			//std::cout << "f constant true NOR false  " << std::endl;
			/* Shannon expansion: f = x f_x + x' f_x' */
			var_t const x = nodes[f.node_index].v;
			index_t const fx = nodes[f.node_index].T.node_index;
			index_t const fnx = nodes[f.node_index].E.node_index;
			uint8_t node_comp = f.complemented ;
			uint8_t E_comp = nodes[f.node_index].E.complemented ;
			//std::cout << "extraction done" << std::endl;
			Truth_Table const tt_x = create_tt_nth_var( num_vars(), x );
			Truth_Table const tt_nx = create_tt_nth_var( num_vars(), x, false );
			//std::cout << "extraction tt " << tt_x.bits.at(0) << " " << tt_nx.bits.at(0) << std::endl;
			if (node_comp == 0 and E_comp == 0 ) {
				//std::cout << "coucou de 1 fx, fnx : " << fx << ", " << fnx << std::endl;
				return ( tt_x & get_tt( edge{fx, false} ) ) | ( ~ tt_x & get_tt( edge{fnx, false} ) );
			}
			else if (node_comp == 0 and E_comp == 1 ) {
				//std::cout << "coucou de 2 fx, fnx : " << fx << ", " << fnx << std::endl;
				return ( tt_x & get_tt( edge{fx, false} ) ) |  ( ~ tt_x & (~ get_tt( edge{fnx, false} ) ) );
			}
			else if (node_comp == 1 and E_comp == 1 ) {
				//std::cout << "coucou de 3 fx, fnx : " << fx << ", " << fnx << std::endl;
				return ~( ( tt_x & get_tt( edge{fx, false} ) ) | ( ~ tt_x & (~ get_tt( edge{fnx, false} ) ) ) );
			}
			else if (node_comp == 1 and E_comp == 0 ) {
				//std::cout << "coucou de 4 fx, fnx : " << fx << ", " << fnx << std::endl;
				return ~( ( tt_x & get_tt( edge{fx, false} ) ) | ( ~ tt_x & get_tt( edge{fnx, false} ) ) );
			}
		}

		/* Whether `f` is dead (having a reference count of 0). */
		bool is_dead( index_t f ) const{
			/* here we need to implement node tracing */
			//std::cout << "we are in is_dead" << std::endl;
			/* TODO */
			
			return ! nodes[f].alive;
		}

		/* Get the number of living nodes in the whole package, excluding constants. */
		uint64_t num_nodes() const
		{
			uint64_t n = 0u;
			for ( auto i = 0; i < nodes.size(); ++i ){
				if ( !is_dead( i ) ){
					++n;
				}
			}
			return n;
		}

	  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
		uint64_t num_nodes( edge f ){
			assert( f.node_index < nodes.size() && "Make sure f exists." );

			if ( IS_EQUAL_EDGE ( f, constant( false ) ) || IS_EQUAL_EDGE(f, constant( true ) ) ){
				return 0u;
			}

			std::vector<bool> visited( nodes.size(), false );
			visited[0] = true;

			return num_nodes_rec( f, visited );
		}

		uint64_t num_invoke() const {
			return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
		}

	private:
		/**********************************************************/
		/******************** Helper Functions ********************/
		/**********************************************************/

		uint64_t num_nodes_rec( edge f, std::vector<bool>& visited ) const {
			assert( f.node_index < nodes.size() && "Make sure f exists." );
		

			uint64_t n = 0u;
			Node const& F = nodes[f.node_index];
			assert( F.T.node_index < nodes.size() && "Make sure the children exist." );
			assert( F.E.node_index < nodes.size() && "Make sure the children exist." );
			if ( !visited[F.T.node_index] ) {
				n += num_nodes_rec( F.T, visited );
				visited[F.T.node_index] = true;
			}
			if ( !visited[F.E.node_index] ) {
				n += num_nodes_rec( F.E, visited );
				visited[F.E.node_index] = true;
			}
			return n + 1u;
		}

	private:
		std::vector<Node> nodes;
		std::vector<std::vector<index_t>> ref_child; /* will be used to know if we can delete a node or not*/
		
		std::vector<index_t> table_empty_node;
		
		std::vector<std::unordered_map<std::tuple<index_t, index_t, bool>, edge>> unique_table;
		/* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
		 * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
		 * See the implementation of `unique` for example usage. */

		/* statistics */
		uint64_t num_invoke_not, num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
	  
		std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t, bool>, edge> computed_table_ITE;
		/* computed_table is used to reduce the calculation time in ITE*/
	   
		std::unordered_map<std::tuple<index_t, bool, index_t, bool, index_t>, edge> computed_table_2op;
		/* computed_table is used to reduce the calculation time in all the others operators*/
};
