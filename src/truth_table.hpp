#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <math.h>

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

/* return i if n == 2^i and i <= 6, 0 otherwise */
inline uint8_t power_two( const uint32_t n )
{
  switch( n )
  {
    case 2u: return 1u;
    case 4u: return 2u;
    case 8u: return 3u;
    case 16u: return 4u;
    case 32u: return 5u;
    case 64u: return 6u;
    case 128u: return 7u;
    case 256u: return 8u;
    case 512u: return 9u;
    case 1024u: return 10u;
    case 2048u: return 11u;
    case 4096u: return 12u;
    case 8192u: return 13u;
    case 16384u: return 14u;
    case 32768u: return 15u;
    case 65536u: return 16u;
    case 131072u: return 17u;
    case 262144u: return 18u;
    case 524288u: return 19u;
    case 1048576u: return 20u;
    case 2097152u: return 21u;
    case 4194304u: return 22u;
    case 8388608u: return 23u;
    case 16777216u: return 24u;
    case 33554432u: return 25u;
    case 67108864u: return 26u;
    case 134217728u: return 27u;
    case 268435456u: return 28u;
    case 536870912u: return 29u;
    case 1073741824u: return 30u;
    case 2147483648u: return 31u;
    default: return 0u;
  }
}

using large_tt = std::vector<uint64_t> ;

class Truth_Table
{
public:
  Truth_Table( uint8_t num_var )
   : num_var( num_var ), bits( 0u )
  {
    assert( num_var <= 32u );
    for (auto i=0; i<=((uint8_t) pow(2,num_var)/pow(2, 6)); i++) //Loop on all the concatenated 6 variables (small) truth tables
	{
		bits.emplace_back(0);
	}
  }

  Truth_Table( uint8_t num_var,  large_tt bits )
   : num_var( num_var )
  {
    assert( num_var <= 32u );
    for (auto i=0; i<=((uint8_t) pow(2,num_var)/pow(2, 6)); i++)
    {
		this->bits.emplace_back(bits[i] & length_mask[n]);
    }
  }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits( 0u )
  {
    if ( num_var == 0u )
    {
      return;
    }

	for(auto i=0; i<(str.size()/64)+1; i++)
	{
		bits.emplace_back(0);
	}
	
    for ( auto i = 0u; i < str.size(); ++i )
    {
      if ( str[i] == '1' )
      {
        set_bit( str.size() - 1 - i );
      }
      else
      {
        assert( str[i] == '0' );
      }
    }
  }

  bool get_bit( uint8_t const pos ) const
  {
    assert( pos < ( 1 << num_var ) );
    auto i=pos/64;
    auto position = pos%64;
    return ( ( bits[i] >> position ) & 0x1 );
  }

  void set_bit( uint8_t const position )
  {
    assert( position < ( 1 << num_var ) );
    bits[position/64] |= ( uint64_t( 1 ) << position%64 );
    bits[position/64]  &= length_mask[n];
  }

  uint8_t n_var() const
  {
    return num_var;
  }


public:
  uint8_t const num_var; /* number of variables involved in the function (large truth table) */
  uint8_t const n = num_var%64;  /* number of variables involved in a small truth table the function */
  large_tt bits; /* the new truth table */
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )
{
  for ( int8_t i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
  {
    os << ( tt.get_bit( i ) ? '1' : '0' );
  }
  return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
	large_tt  tt_not;
   
    for(auto i=0; i < tt.bits.size(); i++)
    {
        tt_not.emplace_back(~tt.bits[i]);
    }
	return Truth_Table( tt.num_var, tt_not );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	assert(tt1.bits.size() == tt2.bits.size());
	large_tt tt_or;
   
	for(auto i=0; i < tt1.bits.size(); i++)
	{
		tt_or.emplace_back(tt1.bits[i] | tt2.bits[i]);
	}
	return Truth_Table( tt1.num_var, tt_or );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	assert( tt1.bits.size() == tt2.bits.size());
	large_tt tt_and;

	for(auto i=0; i < tt1.bits.size(); i++)
	{
		tt_and.emplace_back(tt1.bits[i] & tt2.bits[i]);
	}
	return Truth_Table( tt1.num_var, tt_and );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	assert(tt1.bits.size() == tt2.bits.size());
	large_tt tt_xor;
	for(auto i=0; i < tt1.bits.size(); i++)
	{
		tt_xor.emplace_back(tt1.bits[i] ^ tt2.bits[i]);
	}
	return Truth_Table( tt1.num_var,  tt_xor );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	if ( tt1.num_var != tt2.num_var )
	{
		return false;
	}
	if(tt1.bits.size() != tt2.bits.size())
	{
		return false;
	}
	for(auto i=0; i<tt1.bits.size(); i++)
	{
		if(tt1.bits[i] != tt2.bits[i] )
		return false;
	}
		return true;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return !( tt1.bits == tt2.bits );
}


/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
	assert ( num_var <= 32u && var < num_var );
	large_tt var_long_mask_pos;
	large_tt var_long_mask_neg ;
	for (auto i=0 ; i<num_var %64;i++)
	{
		var_long_mask_pos.emplace_back(var_mask_pos[var]);
	}
	for (auto i=0 ; i<num_var %64;i++)
	{
		var_long_mask_neg.emplace_back(var_mask_neg[var]);
	}
	return Truth_Table( num_var, polarity ? var_long_mask_pos : var_long_mask_neg );
}
