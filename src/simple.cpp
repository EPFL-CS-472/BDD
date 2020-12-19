#include "BDD.hpp"
#include "truth_table.hpp"

#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

void check( Truth_Table const& tt, string const& ans )
{
  cout << "  checking function correctness";
  if ( tt == Truth_Table( ans ) )
  {
    cout << "...passed." << endl;
  }
  else
  {
    cout << "...failed. (expect " << ans << ", but get " << tt << ")" << endl;
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

void check( uint64_t dd_size, uint64_t expected )
{
  cout << "  checking BDD size";
  if ( dd_size == expected )
  {
    cout << "...passed." << endl;
  }
  else
  {
    cout << "...failed. (expect " << expected << ", but get " << dd_size << " nodes)" << endl;
  }
}

int main()
{
  {
    cout << "test 00: x0 XOR x1" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.XOR( x0, x1 );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    cout << tt << endl;
    check( tt, "0110" );
    check( bdd.num_nodes( f ), 3 );
  }

  {
    cout << "test 01: x0 AND x1" << endl;
    BDD bdd( 2 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const f = bdd.AND( x0, x1 );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    cout << tt << endl;
    check( tt, "1000" );
    check( bdd.num_nodes( f ), 2 );
  }

  {
    cout << "test 02: ITE(x0, x1, x2)" << endl;
    BDD bdd( 3 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const x2 = bdd.literal( 2 );
    auto const f = bdd.ITE( x0, x1, x2 );
    auto const tt = bdd.get_tt( f );
    bdd.print( f );
    cout << tt << endl;
    check( tt, "11011000" );
    check( bdd.num_nodes( f ), 3 );
  }
  
  {
    cout << "test 03: XOR(x0, XOR(1, 0))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.XOR( x0, bdd.XOR( 1, 0 ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    check( tt, "01" );
    check( bdd.num_nodes( f ), 1 );
  }

  {
    cout << "test 04: AND(x0, AND(1, 1))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.AND( x0, bdd.AND( 1, 1 ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    check( tt, "10" );
    check( bdd.num_nodes( f ), 1 );
  }
  
  {
    cout << "test 05: AND(x0, AND(0,0))" << endl;
    BDD bdd( 1 );
    auto const x0 = bdd.literal( 0 );
    auto const f = bdd.AND( x0, bdd.AND( 0, 0 ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    check( tt, "00" );
    check( bdd.num_nodes( f ), 0 );
  }
  
  {
    cout << "test 06: XOR(ITE(x0, x1, x2), AND(x0, x3))" << endl;
    BDD bdd( 4 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const x2 = bdd.literal( 2 );
    auto const x3 = bdd.literal( 3 );
    auto const f = bdd.XOR( bdd.ITE( x0, x1, x2 ), bdd.AND( x0, x3 ) );
    auto const tt = bdd.get_tt( f );
    //bdd.print( f );
    //cout << tt << endl;
    check( tt, "0111001011011000" );
    check( bdd.num_nodes( f ), 5 );
  }
  
  {
    cout << "test 07: AND(XOR(AND(x0, AND(x3, x4)), x1), x2)\n\
            == ITE(ITE(ITE(x0, ITE(x3, x4, 0), 0), ITE(x1, 0, 1), x1),\n\t\
                   ITE(x2, 1, 0), ITE(0, ITE(1, 1, 0), 0))" << endl;
    BDD bdd( 5 );
    auto const x0 = bdd.literal( 0 );
    auto const x1 = bdd.literal( 1 );
    auto const x2 = bdd.literal( 2 );
    auto const x3 = bdd.literal( 3 );
    auto const x4 = bdd.literal( 4 );
    
    auto const f = bdd.AND( bdd.XOR( bdd.AND( x0, bdd.AND( x3, x4 ) ), x1 ), x2 );
    auto const g = bdd.ITE( bdd.ITE( bdd.ITE( x0, bdd.ITE( x3, x4, 0 ), 0 ), bdd.ITE( x1, 0, 1 ), x1 ), 
                            bdd.ITE( x2, 1, 0 ), bdd.ITE( 0, bdd.ITE( 1, 1, 0 ), 0 ) );
    auto const tt = bdd.get_tt( f );
    auto const tt2 = bdd.get_tt( g );
    //bdd.print( f );
    //cout << tt << endl;
    check( tt, tt2 );
    check( bdd.num_nodes( f ), bdd.num_nodes( g ) );
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
    check( tt, "1100" );
    check( bdd.num_nodes( f ), 1 );
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
    check( tt, "1110" );
    check( bdd.num_nodes( f ), 2 );
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
    check( tt, "1000" );
    check( bdd.num_nodes( f ), 2 );
  }
  /* Feel free to add more tests. */

    {
    cout << "test 11: AND(AND(x0', x0), x1)" << endl;
    BDD bdd(2);
    auto const x0 = bdd.literal(0);
    auto const x0_n = bdd.literal(0, true);
    auto const x1 = bdd.literal(1);
    auto const f = bdd.AND(bdd.AND(x0, x0_n), x1);
    auto const tt = bdd.get_tt(f);
    //bdd.print( f );
    //cout << tt << endl;
    check(tt, "0000");
    check(bdd.num_nodes(f), 0);
  }

  {
    cout << "test 12: AND(AND(x0', x1), x0)" << endl;
    BDD bdd(2);
    auto const x0 = bdd.literal(0);
    auto const x0_n = bdd.literal(0, true);
    auto const x1 = bdd.literal(1);
    auto const f = bdd.AND(bdd.AND(x0, x1), x0_n);
    auto const tt = bdd.get_tt(f);
    //bdd.print( f );
    //cout << tt << endl;
    check(tt, "0000");
    check(bdd.num_nodes(f), 0);
  }


  {
    cout << "test 13: AND(XOR(AND(x0', x1), x0), XOR(x2', x3))" << endl;
    BDD bdd(4);
    auto const x0 = bdd.literal(0);
    auto const tt_x0 = create_tt_nth_var(4, 0);
    auto const x0_n = bdd.literal(0, true);
    auto const tt_x0_n = create_tt_nth_var(4, 0, false);
    auto const x1 = bdd.literal(1);
    auto const tt_x1 = create_tt_nth_var(4, 1);
    auto const x2_n = bdd.literal(2, true);
    auto const tt_x2_n = create_tt_nth_var(4, 2, false);
    auto const x3 = bdd.literal(3);
    auto const tt_x3 = create_tt_nth_var(4, 3);
    auto const f = bdd.AND(bdd.XOR(bdd.AND(x0_n, x1), x0), bdd.XOR(x2_n, x3));
    auto const tt = bdd.get_tt(f);

    auto const expected_tt = ((tt_x0_n & tt_x1) ^ tt_x0) & (tt_x2_n ^ tt_x3);
    //bdd.print( f );
    //cout << tt << endl;
    check(tt, expected_tt);
    check(bdd.num_nodes(f), 5);
  }

  {
    cout << "test 14: ITE(AND(x0', x1), x0, XOR(x2', x3)) === AND(NOT(AND(x0', x1)), XOR(x2', x3))" << endl;
    BDD bdd(4);
    auto const x0 = bdd.literal(0);
    auto const tt_x0 = create_tt_nth_var(4, 0);
    auto const x0_n = bdd.literal(0, true);
    auto const tt_x0_n = create_tt_nth_var(4, 0, false);
    auto const x1 = bdd.literal(1);
    auto const tt_x1 = create_tt_nth_var(4, 1);
    auto const x2_n = bdd.literal(2, true);
    auto const tt_x2_n = create_tt_nth_var(4, 2, false);
    auto const x3 = bdd.literal(3);
    auto const tt_x3 = create_tt_nth_var(4, 3);
    auto const f = bdd.ITE(bdd.AND(x0_n, x1), x0, bdd.XOR(x2_n, x3));
    auto const tt_t = bdd.get_tt(f);

    auto const g = bdd.AND(bdd.NOT(bdd.AND(x0_n, x1)), bdd.XOR(x2_n, x3));
    auto const tt_g = bdd.get_tt(g);

    auto const expected_tt = ((tt_x0_n & tt_x1) & tt_x0) | ((~(tt_x0_n & tt_x1)) & (tt_x2_n ^ tt_x3));

    check(tt_t, expected_tt);
    check(bdd.num_nodes(f), bdd.num_nodes(g));
    assert(f == g); // Should fail if optimzation is not correct, f and g should be the same
  }

  return 0;
}
