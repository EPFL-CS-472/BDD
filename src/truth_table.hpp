#pragma once

#include <iostream>
#include <cassert>
#include <string>

//#include "../kitty/include/kitty/kitty.hpp"

/* masks used to filter out unused bits */
static const uint64_t length_mask[] = {
  0x0000000000000001,
  0x0000000000000003,
  0x000000000000000f,
  0x00000000000000ff,
  0x000000000000ffff,
  0x00000000ffffffff,
  0xffffffffffffffff};

/* masks used to get the bits where a certain variable is 1 */
static const uint64_t var_mask_pos[] = {
  0xaaaaaaaaaaaaaaaa,
  0xcccccccccccccccc,
  0xf0f0f0f0f0f0f0f0,
  0xff00ff00ff00ff00,
  0xffff0000ffff0000,
  0xffffffff00000000};

/* masks used to get the bits where a certain variable is 0 */
static const uint64_t var_mask_neg[] = {
  0x5555555555555555,
  0x3333333333333333,
  0x0f0f0f0f0f0f0f0f,
  0x00ff00ff00ff00ff,
  0x0000ffff0000ffff,
  0x00000000ffffffff};

/* return i if n == 2^i, 0 otherwise */
inline uint32_t power_two( const uint32_t n )
{
  if (ceil(log2(n)) == floor(log2(n)))
      return (uint32_t)log2(n);
  else
      return 0u;
}

class Truth_Table
{
public:
  
  Truth_Table(uint8_t num_var)
  {
    kitty::dynamic_truth_table tt(num_var);
    tt_fun = tt;
  }

  Truth_Table(uint8_t num_var, uint64_t bits)
  {
    kitty::dynamic_truth_table tt(num_var);
    kitty::create_from_binary_string(tt, std::to_string(bits));
    tt_fun = tt;
  }

  Truth_Table( const std::string str )
  {
    kitty::dynamic_truth_table tt(power_two(str.size()));
    kitty::create_from_binary_string(tt, str);
    tt_fun = tt;
  }

  Truth_Table( kitty::dynamic_truth_table tt )
  {
      tt_fun = tt;
  }

  uint8_t n_var() const
  {
      return tt_fun.num_vars();
  }

  bool get_bit( uint8_t const position ) const
  {
    assert( position < ( 1 << n_var() ) );
    return kitty::get_bit(tt_fun, position);
  }

  void set_bit( uint8_t const position )
  {
    assert( position < ( 1 << n_var() ) );
    kitty::set_bit(tt_fun, position);
  }

  Truth_Table positive_cofactor( uint8_t const var ) const;
  Truth_Table negative_cofactor( uint8_t const var ) const;
  Truth_Table derivative( uint8_t const var ) const;
  Truth_Table consensus( uint8_t const var ) const;
  Truth_Table smoothing( uint8_t const var ) const;

public:
  kitty::dynamic_truth_table tt_fun; /* the truth table of the function */
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )
{
  for ( int8_t i = ( 1 << tt.tt_fun.num_vars() ) - 1; i >= 0; --i )
  {
    os << ( tt.get_bit( i ) ? '1' : '0' );
  }
  return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
  return Truth_Table(kitty::unary_not(tt.tt_fun));
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.tt_fun.num_vars() == tt2.tt_fun.num_vars() );
  return Truth_Table(kitty::binary_or(tt1.tt_fun, tt2.tt_fun));
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.tt_fun.num_vars() == tt2.tt_fun.num_vars() );
  return Truth_Table(kitty::binary_and(tt1.tt_fun, tt2.tt_fun));
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.tt_fun.num_vars() == tt2.tt_fun.num_vars() );
  return Truth_Table(kitty::binary_xor(tt1.tt_fun, tt2.tt_fun));
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return (kitty::equal(tt1.tt_fun, tt2.tt_fun) == true);
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return (kitty::equal(tt1.tt_fun, tt2.tt_fun) == false);
}

inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
{
  assert( var < n_var() );
  return Truth_Table(cofactor1(tt_fun, var));
}

inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
{
  assert( var < n_var() );
  return Truth_Table(cofactor0(tt_fun, var));
}

inline Truth_Table Truth_Table::derivative( uint8_t const var ) const
{
  assert( var < n_var() );
  return positive_cofactor( var ) ^ negative_cofactor( var );
}

inline Truth_Table Truth_Table::consensus( uint8_t const var ) const
{
  assert( var < n_var() );
  return positive_cofactor( var ) & negative_cofactor( var );
}

inline Truth_Table Truth_Table::smoothing( uint8_t const var ) const
{
  assert( var < n_var() );
  return positive_cofactor( var ) | negative_cofactor( var );
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
  assert ( var < num_var );

  Truth_Table tt = Truth_Table(num_var);
  kitty::create_nth_var(tt.tt_fun, var, !polarity);
  return tt;
}