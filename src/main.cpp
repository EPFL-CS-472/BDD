#include "BDD.hpp"
#include "truth_table.hpp"

#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

bool check( Truth_Table const& tt, string const& ans )
{
  cout << "  checking function correctness";
  if ( tt == Truth_Table( ans ) )
  {
    cout << "...passed." << endl;
    return true;
  }
  else
  {
    cout << "...failed. (expect " << ans << ", but get " << tt << ")" << endl;
    return false;
  }
}

void check( Truth_Table const& tt, Truth_Table const& tt2 )
{
  cout << "  checking function correctness";
  if ( tt == tt2 )
  {
    cout << "...passed." << endl;
  }
  else
  {
    cout << "...failed. (expect " << tt2 << ", but get " << tt << ")" << endl;
  }
}

bool check( uint64_t actual, uint64_t expected )
{
  if ( actual <= expected )
  {
    cout << "...passed." << endl;
    return true;
  }
  else
  {
    cout << "...failed. (expect " << expected << ", but get " << actual << ")" << endl;
    return false;
  }
}

ostream& operator<<(ostream& os, const struct BDD::Node& book) {
    return os << "Value: " << book.v << endl
              << "Then: " << book.T.child << " inv : " << book.T.inv << endl
              << "Else: " << book.E.child << " inv : " << book.E.inv << endl;

}

void print(std::vector<struct BDD::Node> const &input, std::ostream& os = std::cout)
{
    for (uint64_t i = 0u; i < input.size(); i++) {
        os << i << input.at(i) << endl;
    }
}

ostream& operator<<(ostream& os, const std::unordered_map<std::pair<BDD::index_t, BDD::index_t>, BDD::index_t>& book) {
    for(auto it : book){
     /* os << "    T :" << it.first.first.child << " inv : " << it.first.first.inv  << " E :" << it.first.second.child;
        os << " inv : " << it.first.second.inv << " Node :" << it.second.child << " inv : " << it.second.inv << endl;
    */
        os << "    T :" << it.first.first << " E :" << it.first.second;
                os << " Node :" << it.second << endl;
    }

    return os;
}

void print(std::vector<std::unordered_map<std::pair<BDD::index_t, BDD::index_t>, BDD::index_t>> map, std::ostream& os = std::cout)
{
    for (uint64_t i = 0u; i < map.size(); i++) {
        os << i << endl;
        os << map.at(i) << endl;
    }
}

int main()
{
  bool passed = true;
  /*{
    cout << "test 00: large truth table";
    Truth_Table tt( "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" );

    if ( tt.n_var() == 7 )
    {
      cout << "...passed." << endl;
    }
    else
    {
      cout << "...failed." << endl;
      passed = false;
    }
  }
*/
  {
    cout << "test 01: computed table" << endl;
    BDD bdd( 3 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.XOR( bdd.AND( x0, x1 ), bdd.AND( x0, x1 ) );
    auto const tt = bdd.get_tt( f );

    passed &= check( tt, "0000" );
    cout << "  checking number of computation";
    passed &= check( bdd.num_invoke(), 4 );
    cout << tt << endl;

  }

  {
    cout << "test 02: complemented edges" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.XOR( x0, x1 );
    auto const tt = bdd.get_tt( f );
    bdd.print(f);
    bdd.cce_conversion();
    bdd.count_references();
    bdd.print(f);
    passed &= check( tt, "0110" );
    cout << "  checking BDD size (reachable nodes)";
    passed &= check( bdd.num_nodes( f ), 2 );
  }

  {
    cout << "test 03: reference count" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.XOR( x0, x1 );
    auto const tt = bdd.get_tt( f );

    passed &= check( tt, "0110" );
    cout << "  checking BDD size (living nodes)";
    passed &= check( bdd.num_nodes(), 2 );
  }
  {
    cout << "test 00: x0 XOR x1" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.XOR( x0, x1 );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    cout << tt << endl;
    passed &=check( tt, "0110" );
    passed &=check( bdd.num_nodes( f ), 3 );
  }

  {
    cout << "test 01: x0 AND x1" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.AND( x0, x1 );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "1000" );
    passed &=check( bdd.num_nodes( f ), 2 );
  }

  {
    cout << "test 02: ITE(x0, x1, x2)" << endl;
    BDD bdd( 3 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const x2 = bdd.literal( 2 );
    auto const f = bdd.ITE( x0, x1, x2 );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "11011000" );
    passed &=check( bdd.num_nodes( f ), 3 );
  }

  {
    cout << "test 03: XOR(x0, XOR(1, 0))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.XOR( x0, bdd.XOR( {0,1}, {0,0} ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "01" );
    passed &=check( bdd.num_nodes( f ), 1 );
  }

  {
    cout << "test 04: AND(x0, AND(1, 1))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.AND( x0, bdd.AND( {0,1}, {0,1} ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "10" );
    passed &=check( bdd.num_nodes( f ), 1 );
  }

  {
    cout << "test 05: AND(x0, AND(0, 0))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.AND( x0, bdd.AND({0, 0}, {0,0} ) );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    cout << tt << endl;
    passed &=check( tt, "00" );
    passed &=check( bdd.num_nodes( f ), 0 );
  }

  {
    cout << "test 06: XOR(ITE(x0, x1, x2), AND(x0, x3))" << endl;
    BDD bdd( 4 );
    auto const x0 = bdd.literal( 0 );
   // print(bdd.nodes);
    auto const x1 = bdd.literal( 1 );
  //  print(bdd.nodes);
    auto const x2 = bdd.literal( 2 );
   // print(bdd.nodes);
    auto const x3 = bdd.literal( 3 );
   // print(bdd.nodes);
    auto const g  = bdd.AND(x0, x3);
   // print(bdd.nodes);
    auto const h  = bdd.ITE(x0, x1, x2);
   // print(bdd.nodes);
    auto const f = bdd.XOR( bdd.ITE( x0, x1, x2 ), bdd.AND( x0, x3 ) );
   // print(bdd.nodes);
   // print(bdd.unique_table);
    auto const tt = bdd.get_tt( f );
    cout << tt << endl;
   // print(bdd.nodes);


    cout << f.child << " inv: " << f.inv << endl;
    passed &=check( tt, "0111001011011000" );
    passed &=check( bdd.num_nodes( f ), 5 );
  }

  {
    cout << "test 08: ITE(x0, x1, x1)" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.ITE( x0, x1, x1 );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "1100" );
    passed &=check( bdd.num_nodes( f ), 1 );
  }

  {
    cout << "test 09: ITE(x0, x0, x1)" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.ITE( x0, x0, x1 );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "1110" );
    passed &=check( bdd.num_nodes( f ), 2 );
  }

  {
    cout << "test 10: ITE(x0, x1, x0)" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.ITE( x0, x1, x0 );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    passed &=check( tt, "1000" );
    passed &=check( bdd.num_nodes( f ), 2 );
  
  }

  {
    cout << "test 10: ITE(x0, x1, x0)" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.OR( x0, x1 );
    auto const g = bdd.XOR( x0, x1);
    auto const h = bdd.ITE( x0, f, g);
    auto const tt = bdd.get_tt( h );
    bdd.print( h );
    cout << tt << endl;
   // passed &=check( tt, "1000" );
  //  passed &=check( bdd.num_nodes( f ), 2 );

  }
  return passed ? 0 : 1;
}
