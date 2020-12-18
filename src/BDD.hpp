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

  using edge = uint32_t; // represents an edge
  using index_t = uint32_t; // represents a node
  using var_t = uint32_t; // represents a variable
  /* Similarly, declare `var_t` also as an alias for an unsigned integer.
   * This datatype will be used for representing variables. */

private:
  struct Node //struct that def a Node
  {
      var_t v; /* corresponding variable */
      edge T; /* index of THEN child */
      edge E; /* index of ELSE child */
      
      
      edge nce; //non complemented edge
      edge ce; //complemented edge
      uint32_t nb_pointed; //instanciation of a node
      
  };
    
    
    //new external structure to treat a node
  struct reference {
    bool c; // is complemented or not
    index_t idNode; //index of a node
  };
    
    
    
public:
  explicit BDD( uint32_t num_vars ) //initialization of BDD
    : unique_table( num_vars ), //initialization of unique_table
    // initialization all num_invoke start to 0
    num_invoke_and( 0u ),
    num_invoke_or( 0u ),
    num_invoke_xor( 0u ),
    num_invoke_ite( 0u )
  {
    //Initialization of the leaf: only one is needed for complemented graph
    nodes.emplace_back(Node({num_vars, 1, 1, 1, 0, 0})); /* constant 1 */
    //external referencement of the leaf
    refs.emplace_back(reference{true, 0}); // complemented edge initialized
    refs.emplace_back(reference{false, 0}); // non complemented edge initialized
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
    edge constant( bool value ) const
  {
    return value ? 1 : 0; //if value is true return 1 else 0
  }

    
  /* Look up (if exist) or build (if not) the node with variable `var`,
   * THEN child `T`, and ELSE child `E`. */
  edge unique( var_t var, edge T, edge E )
  {
 
    /* Reduction rule: Identical children */
    if ( T == E ) // if T is E then reference T edge
    {
      return ref(T); //reference T
    }

    bool test_c = false; //test if complemented // memory
    if(refs[T].c)//if the ref of T is complemented then inverse it and complement E
    {
      test_c = true;
      T = newref(false, refs[T].idNode); //create new ref for the node
      E = complemented(E); //create a new ref for the complement
    }

    /* Look up in the unique table. */
    const auto pair_exist = unique_table[var].find( {T, E} );//search pair T E in the unique table and put it in it
    if ( pair_exist != unique_table[var].end() ) // if pair is in unique
    {
        //then build the ref
      return newref(test_c, pair_exist->second);

    }
      
    else //the node is not in unique I can ref and create new node and insert it in unique
    {
      /* Create a new node and insert it to the unique table. */
      ref(T); //referencement of T +1 to Node.nb_pointed
      ref(E); //idem
      index_t const id = nodes.size(); // index of the node
      nodes.emplace_back( Node({var, T, E, 100000, 100000, 0}) ); //MODIF nodes.emplace_back( Node({var, T, E}) );
      unique_table[var][{T, E}] = id; //put id in unique table
      return newref(test_c, id); //reference node with new id
    }
  }


  /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
    edge literal( var_t var, bool complement = false )
  {
    return unique( var, constant( !complement ), constant( complement ));
  }
    
    index_t ref(index_t e)
    {
       nodes[refs[e].idNode].nb_pointed = nodes[refs[e].idNode].nb_pointed + 1; //+1 to the node referenced pointed by e
      return e;
    }

    void deref(index_t e)
    {
        // do nothing for e=0 and e=1
        if(e== 0)
        {
            return ;
        }
        if(e== 1)
        {
            return ;
        }
        //else
        nodes[refs[e].idNode].nb_pointed =nodes[refs[e].idNode].nb_pointed -1; //-1 to node referenced
        
        //recursive way to 'kill', will not be counted as an active node
        //nodes bellow and just linked one time by that node will also be killed because nb_pointed goes to 0
          if(nodes[refs[e].idNode].nb_pointed== 0)
          {
          deref(nodes[refs[e].idNode].T);
          deref(nodes[refs[e].idNode].E);
          }
    }

    
  /**********************************************************/
  /********************* BDD Operations *********************/
  /**********************************************************/

  /* Compute ~f */
    edge NOT( edge f )
  {
    return complemented(f); //return the complement
  }
  /* Compute f ^ g */
    edge XOR( edge f, edge g )
  {
    ++num_invoke_xor;
    edge r;//my result of XOR operation
      
    /* trivial cases */
    if ( f == g ) //if f=g return 0
    {
      r = constant( false );
    }
    else if ( f == constant( false ) ) //if f=0 then result is g
    {
      r = g;
    }
    else if ( g == constant( false ) ) //if g=0
    {
      r = f;
    }
    else if ( f == constant( true ) ) //if f is 1 return NOT g
    {
      r = NOT( g );
    }
    else if ( g == constant( true ) ) //if g is 1 return not f
    {
      r = NOT( f );
    }
    else if ( f == NOT( g ) ) //if f is not g return 1
    {
      r = constant( true );
    }
    else
    {
      reference refF = refs[f]; //reference to edge f
      reference refG = refs[g]; // ref to edge g
      Node &F = nodes[refF.idNode]; //node associated to edge f
      Node &G = nodes[refG.idNode]; // node associated to edge g

      var_t x; // the variable
      edge f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E; //if edge complemented f0 takes complemented(F.E) else takes F.E
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }
        edge r0;
        edge r1;

      auto r0_exist = isIn(unique_xor, f0, g0);
      if(r0_exist.first){
        r0 = r0_exist.second;
      } else {
        r0 = XOR( f0, g0 );
      }

      auto r1_exist = isIn(unique_xor, f1, g1);
      if(r1_exist.first){
        r1 = r1_exist.second;
      } else {
        r1 = XOR(f1, g1);
      }
      r = unique(x, r1, r0);
    }
      
      //store in the computed table XOR
      unique_xor[std::make_pair(f, g)] = r;
      unique_xor[std::make_pair(complemented(f), g)] = complemented(r); //additional to pass the test
      
      
      
    return r;
  }

    
    

  /* Compute f & g */
    edge AND( edge f, edge g )
  {
    
    ++num_invoke_and;
    edge r;

    /* trivial cases */
    if ( f == constant( false ) || g == constant( false ) ){
      r = constant( false );
    } else if ( f == constant( true ) ){
      r = g;
    } else if ( g == constant( true ) ){
      r = f;
    } else if ( f == g ){
      r = f;
    } else {
        reference refF = refs[f];
        reference refG = refs[g];

      Node &F = nodes[refF.idNode];
      Node &G = nodes[refG.idNode];

      var_t x;
        edge f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top of G */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }

        edge r0, r1;
      auto r0_exist = isIn(unique_and, f0, g0);
      if(r0_exist.first){
        r0 = r0_exist.second;
      } else {
        r0 = AND( f0, g0 );
      }
      auto r1_exist = isIn(unique_and, f1, g1);
      if(r1_exist.first){
        r1 = r1_exist.second;
      } else {
        r1 = AND( f1, g1 );
      }
      r = unique(x, r1, r0);
    }

    
      //store in the computed table AND
      unique_and[std::make_pair(f, g)] = r;
      
    return r;
  }

  /* Compute f | g */
    edge OR( edge f, edge g )
  {
    
    ++num_invoke_or;

    edge r;

    /* trivial cases */
    if ( f == constant( true ) || g == constant( true ) ){
      r = constant( true );
    } else if ( f == constant( false ) ){
      r = g;
    } else if ( g == constant( false ) ){
      r = f;
    } else if ( f == g ) {
      r = f;
    } else {
        reference refF = refs[f];
        reference refG = refs[g];

      Node &F = nodes[refF.idNode];
      Node &G = nodes[refG.idNode];

      var_t x;
        edge f0, f1, g0, g1;
      if ( F.v < G.v ) /* F is on top ofm G */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = g1 = g;
      }
      else if ( G.v < F.v ) /* G is on top of F */
      {
        x = G.v;
        f0 = f1 = f;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }
      else /* F and G are at the same level */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        g0 = refG.c ? complemented(G.E) : G.E;
        g1 = refG.c ? complemented(G.T) : G.T;
      }

        edge r0, r1;
       //Protection to avoid recall OR if value already stored in computed
      //auto look_r0 = isIn(or_uni, f0, g0);
      if(isIn(unique_or, f0, g0).first){ //if f0 and g0 are in the computed table .first gives true
        r0 = isIn(unique_or, f0, g0).second; //then assign the value given by the table
      }
      else
      //else call OR
      {
        r0 = OR( f0, g0 );
      }
      //auto look_r1 = isIn(unique_or, f1, g1);
      //Same idea for pair f1 g1
      if( isIn(unique_or, f1, g1).first)
      {
        r1 = isIn(unique_or, f1, g1).second;
      }
      else
      //else call OR
      {
        r1 = OR( f1, g1 );
      }
      //compute unique
      r = unique(x, r1, r0);
    }

   
      
      //store in the computed table OR
      unique_or[std::make_pair(f, g)] = r;
      

    return r;
  }

  /* Compute ITE(f, g, h), i.e., f ? g : h */
    edge ITE( edge f, edge g, edge h )
  {
    
    ++num_invoke_ite;

      edge r;
    
    /* trivial cases */
    if ( f == constant( true ) ){
      r = g;
    } else if ( f == constant( false ) ){
      r = h;
    } else if ( g == h ){
      r = g;
    } else {
        reference &refF = refs[f];
        reference &refG = refs[g];
        reference &rH = refs[h];

      Node &F = nodes[refF.idNode];
      Node &G = nodes[refG.idNode];
      Node &H = nodes[rH.idNode];
        

      var_t x;
        edge f0, f1, g0, g1, h0, h1;
      if ( F.v <= G.v && F.v <= H.v ) /* F is not lower than both G and H */
      {
        x = F.v;
        f0 = refF.c ? complemented(F.E) : F.E;
        f1 = refF.c ? complemented(F.T) : F.T;
        if ( G.v == F.v )
        {
          g0 = refG.c ? complemented(G.E) : G.E;
          g1 = refG.c ? complemented(G.T) : G.T;
        }
        else
        {
          g0 = g1 = g;
        }
        if ( H.v == F.v )
        {
          h0 = rH.c ? complemented(H.E) : H.E;
          h1 = rH.c ? complemented(H.T) : H.T;
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
          g0 = refG.c ? complemented(G.E) : G.E;
          g1 = refG.c ? complemented(G.T) : G.T;
          h0 = h1 = h;
        }
        else if ( H.v < G.v )
        {
          x = H.v;
          g0 = g1 = g;
          h0 = rH.c ? complemented(H.E) : H.E;
          h1 = rH.c ? complemented(H.T) : H.T;
        }
        else /* G.v == H.v */
        {
          x = G.v;
          g0 = refG.c ? complemented(G.E) : G.E;
          g1 = refG.c ? complemented(G.T) : G.T;
          h0 = rH.c ? complemented(H.E) : H.E;
          h1 = rH.c ? complemented(H.T) : H.T;
        }
      }

        edge r0, r1;

      
      auto r0_exist = unique_ite.find( std::make_tuple(f0, g0, h0) ); //research tuple f0 g0 h0 in the computed table
      if(r0_exist != unique_ite.end()) //if exist
      {
        r0 = r0_exist->second; //return its value
      }
      else
      {
        r0 = ITE( f0, g0, h0 ); //if not recall ite
      }

      
      auto r1_search = unique_ite.find( std::make_tuple(f1, g1, h1) ); //research tuple f1 g1 h1 in the computed table
      if(r1_search != unique_ite.end()) //if exist
      {
        r1 = r1_search->second; //return its value
      }
      else
      {
        r1 = ITE(f1, g1, h1); // if not recall ite
      }

      r = unique(x, r1, r0);
    }

      unique_ite[std::make_tuple(f, g, h)] = r; //store computed value in ite computed table
      unique_ite[std::make_tuple(complemented(f), h, g)] = r; //store another calculated case to pass the test (expect <= 10, but get 14)
      
    return r;
  }

  /**********************************************************/
  /***************** Printing and Evaluating ****************/
  /**********************************************************/

  /* Print the BDD rooted at node `f`. */
  void print( edge f, std::ostream& os = std::cout ) const
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
  Truth_Table get_tt( edge f ) const
  {
    

    if ( f == constant( false ) )
    {
      return Truth_Table( num_vars() );
    }
    else if ( f == constant( true ) )
    {
      return ~Truth_Table( num_vars() );
    }

      
    //Interface to call a node as before in the Shannon expension
    reference ref_f = refs[f];
    Node node_f = nodes[ref_f.idNode];
    /* Shannon expansion: f = x f_x + x' f_x' */
    var_t x = node_f.v;
    index_t fx = node_f.T;
    index_t fnx = node_f.E;
    Truth_Table tt_x = create_tt_nth_var( num_vars(), x );
    Truth_Table tt_nx = create_tt_nth_var( num_vars(), x, false );
      
    auto r = ( tt_x & get_tt( fx ) ) | ( tt_nx & get_tt( fnx ) );
    if(ref_f.c)
    {
      return ~r;
    }
    return r;
  }

  /* Get the number of living nodes in the whole package, excluding constants. */
  uint64_t num_nodes() const
  {
    uint64_t n = 0u;
    for ( auto i = 1u; i < nodes.size(); ++i )//for all nodes
    {
      if ( nodes[i].nb_pointed != 0) //if nb_pointed of a node is not 0 then n++
      {
        ++n;
      }
    }
    return n;
  }

  /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
  uint64_t num_nodes( edge f ) const
  {
   

    if ( f == constant( false ) || f == constant( true ) )
    {
      return 0u;
    }



    std::vector<bool> visited( nodes.size(), false );
    visited[0] = true;
    visited[1] = true;

    return num_nodes_rec( refs[f].idNode, visited ); //MODIF (f, visited);
  }

  uint64_t num_invoke() const
  {
      return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
     
  }


  /**********************************************************/
  /******************** Helper Functions ********************/
  /**********************************************************/

    
    //MODIF
  uint64_t num_nodes_rec( index_t f, std::vector<bool>& visited ) const
  {
    uint64_t n = 0u;
    Node const& F = nodes[f];
    
    index_t id_t = refs[F.T].idNode; //interface with ref
    
    if ( !visited[id_t] )
    {
      n += num_nodes_rec( id_t, visited );
      visited[id_t] = true;
    }
    index_t id_e = refs[F.E].idNode; //interface with ref
    assert(id_e < nodes.size() && "Make sure the children point to a valid node.");
    if ( !visited[id_e] )
    {
      n += num_nodes_rec( id_e, visited );
      visited[id_e] = true;
    }
    return n + 1u;
  }

    
    //MODIF
    edge complemented(edge ref)
    {
    return newref(!refs[ref].c, refs[ref].idNode);
    }

    
    //MODIF
    edge newref(bool c, index_t idNode)
    {
    Node &node = nodes[idNode];
        if(c && node.ce != 100000) //if node is referenced and complemented //large number 100000 means not referenced
        {
            return node.ce; // return complemented from node
        }
        if (!c && node.nce != 100000) //if node is referenced and complemented //large number 100000 means not referenced
        {
            return node.nce; //return non complemented from node
        }
    else //else
    {
      index_t r = refs.size();
      refs.emplace_back(reference{c, idNode}); //reference new node
      if(c) //if complmeneted
      {
        node.ce = r; //put the idx in the complemented
      }
      else
      {
        node.nce = r; //put the idx in the non complemented
      }
      return r;
    }
  }


  /* statistics */
  uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
    std::vector<Node> nodes; //vector store nodes
    std::vector<reference> refs; //vectror that store all my references to nodes
    std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table; //store unicity of a node
    /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
     * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
     * See the implementation of `unique` for example usage. */
    std::unordered_map<std::pair<edge, edge>, edge> unique_and; //store computed and
    std::unordered_map<std::pair<edge, edge>, edge> unique_or; //store computed or
    std::unordered_map<std::pair<edge, edge>, edge> unique_xor; //store computed xor
    std::unordered_map<std::tuple<edge, edge, edge>, edge> unique_ite; //store computed ite
    
    //see if a pair f g is in the computed table
    std::pair<bool, edge> isIn(const std::unordered_map<std::pair<edge, edge>, edge> &table, edge f, edge g) const {
      if(table.find(std::make_pair(f, g)) != table.end()){ //if in
        return std::make_pair(true, table.find(std::make_pair(f, g))->second); //return the computed
      }
      return std::make_pair(false, 0); //return vecto that indicate not computed yet
    }
};




