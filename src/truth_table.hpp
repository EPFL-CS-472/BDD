#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include <cmath>

/* masks used to filter out unused bits */
static const uint64_t length_mask_table[] = {
  0x0000000000000001,
  0x0000000000000003,
  0x000000000000000f,
  0x00000000000000ff,
  0x000000000000ffff,
  0x00000000ffffffff,
  0xffffffffffffffff};

/* masks used to get the bits where a certain variable is 1 */
static const uint64_t var_mask_pos_table[] = {
  0xaaaaaaaaaaaaaaaa,
  0xcccccccccccccccc,
  0xf0f0f0f0f0f0f0f0,
  0xff00ff00ff00ff00,
  0xffff0000ffff0000,
  0xffffffff00000000};

/* masks used to get the bits where a certain variable is 0 */
static const uint64_t var_mask_neg_table[] = {
  0x5555555555555555,
  0x3333333333333333,
  0x0f0f0f0f0f0f0f0f,
  0x00ff00ff00ff00ff,
  0x0000ffff0000ffff,
  0x00000000ffffffff};

/* Returns mask used to filter out unused bits for a certain number of var */
inline std::vector<uint64_t> length_mask(uint8_t const num_var, uint64_t size) 
{
  if (num_var <= 6) 
  {
    std::vector<uint64_t> mask(1, length_mask_table[num_var]);
    return mask;
  }
  else 
  {
    std::vector<uint64_t> mask(size, 63u);
    return mask;
  }
}

/* Returns mask used to get the bits where a certain variable is 1 */
inline std::vector<uint64_t> var_mask_pos(uint8_t const num_var, uint8_t vars) 
{
  uint64_t size = 1u;
  if (num_var <= 6) 
  {
    std::vector<uint64_t> mask(size, var_mask_pos_table[num_var]);
    return mask;
  }
  uint64_t shift = vars - 6;
  if (shift < 1) std::cout << "NYU1" << std::endl; 
  size = size << shift;
  uint64_t half_blocks = vars - 7;
  if (half_blocks < 1) std::cout << "NYU2" << std::endl; 
  std::vector<uint64_t> mask;
  uint64_t ones, zeroes = (1 << half_blocks);
  for (auto b = 0u; b < size; b += ones + zeroes) {
    for (auto i = 0u; i < ones; i++) {
      mask.push_back(63u);
    }
    for (auto i = 0u; i < zeroes; i++) {
      mask.push_back(0u);
    }
    std::cout << "NYU3 " << b << std::endl; 
  }
  return mask;
}

/* Returns mask used to get the bits where a certain variable is 0 */
inline std::vector<uint64_t> var_mask_neg(uint8_t const num_var, uint64_t vars) 
{
  uint64_t size = 1u;
  if (num_var <= 6) 
  {
    std::vector<uint64_t> mask(size, var_mask_neg_table[num_var]);
    return mask;
  }
  uint64_t shift = vars - 6;
  if (shift < 1) std::cout << "NYU1" << std::endl; 
  size = size << shift;
  uint64_t half_blocks = vars - 7;
  if (half_blocks < 1) std::cout << "NYU2" << std::endl; 
  std::vector<uint64_t> mask;
  uint64_t ones, zeroes = (1 << half_blocks);
  for (auto b = 0u; b < size; b += ones + zeroes) {
    for (auto i = 0u; i < zeroes; i++) {
      mask.push_back(0u);
    }
    for (auto i = 0u; i < ones; i++) {
      mask.push_back(63u);
    }
    std::cout << "NYU3 " << b << std::endl; 
  }
  return mask;
}

/* return i if n == 2^i, 0 otherwise */
inline uint8_t power_two( const uint32_t n, uint8_t acc )
{
    // 1 is the only odd number which is a power of 2 (2^0) 
    if (n == 1) 
      return acc; 
     
    // all other odd numbers, and 0, are not powers of 2
    else if (n % 2 != 0 || n == 0) 
      return 0u; 
     
    return power_two(n / 2, acc + 1); 
}

class Truth_Table
{
public:
  Truth_Table( uint8_t num_var )
   : num_var( num_var ) { 
     bits.push_back(0u);
   }

  /*Truth_Table( uint8_t num_var, uint64_t bits )
   : num_var( num_var ), bits( bits & length_mask[num_var] ) { }*/

  Truth_Table( uint8_t num_var, const std::vector<uint64_t>& bits )
   : num_var( num_var ), bits( bits ) { }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size(), 0u ) )
  {
    if ( num_var == 0u )
    {
      return;
    }
    uint64_t init_block = 0u; 
    bits.push_back(init_block);
    uint64_t block_ind = 0;
    for ( auto i = 0u; i < str.size(); ++i )
    {
      if (i % 64 >= bits.size() * 64) {
        block_ind += 1;
        bits.push_back(init_block);
      }
      if ( str[i] == '1' )
      {
        set_bit( block_ind, (str.size() - 1 - i) % 64 );
      }
      else
      {
        assert( str[i] == '0' );
      }
    }
  }

  bool get_bit( uint64_t block, uint8_t const position ) const
  {
    //assert( position < ( 1 << num_var ) );
    return ( ( bits[block] >> position ) & 0x1 );
  }

  void set_bit( uint64_t block, uint8_t const position )
  {
    //assert( position < ( 1 << num_var ) );
    bits[block] |= ( uint64_t( 1 ) << position );
    auto mask = length_mask(num_var, bits.size());
    for (auto i = 0; i < bits.size(); ++i) {
      bits[i] &= mask[i];
    }
  }

  uint8_t n_var() const
  {
    return num_var;
  }

  //Truth_Table positive_cofactor( uint8_t const var ) const;
  //Truth_Table negative_cofactor( uint8_t const var ) const;
  //Truth_Table derivative( uint8_t const var ) const;
  //Truth_Table consensus( uint8_t const var ) const;
  //Truth_Table smoothing( uint8_t const var ) const;

public:
  uint8_t const num_var; /* number of variables involved in the function */
  std::vector<uint64_t> bits; /* the truth table */
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )
{
  for (auto b = 0u; b < tt.bits.size(); ++b) {
    for ( int8_t i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
    {
      os << ( tt.get_bit( b, i ) ? '1' : '0' );
    }
  }
  return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
  std::vector<uint64_t> tt_bits;
  for (auto i = 0u; i < tt.bits.size(); ++i) {
    tt_bits.push_back(~tt.bits[i]);
  }
  return Truth_Table( tt.num_var, tt_bits );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> tt_bits;
  for (auto i = 0u; i < tt1.bits.size(); ++i) {
    tt_bits.push_back(tt1.bits[i] | tt2.bits[i]);
  }
  return Truth_Table( tt1.num_var, tt_bits );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> tt_bits;
  for (auto i = 0u; i < tt1.bits.size(); ++i) {
    tt_bits.push_back(tt1.bits[i] & tt2.bits[i]);
  }
  return Truth_Table( tt1.num_var, tt_bits );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> tt_bits;
  for (auto i = 0u; i < tt1.bits.size(); ++i) {
    tt_bits.push_back(tt1.bits[i] ^ tt2.bits[i]);
  }
  return Truth_Table( tt1.num_var, tt_bits );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  if ( tt1.num_var != tt2.num_var )
  {
    return false;
  }
  for (auto i = 0u; i < tt1.bits.size(); i++) {
    auto mask = length_mask(tt1.num_var, tt1.bits.size());
    if ( ( tt1.bits[i] & mask[i] ) != ( tt2.bits[i] & mask[i] ) ) 
    {
      return false;
    }
  }
  return true;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return !( tt1 == tt2 );
}

/*inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  return Truth_Table( num_var, ( bits & var_mask_pos[var] ) | ( ( bits & var_mask_pos[var] ) >> ( 1 << var ) ) );
}

inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  return Truth_Table( num_var, ( bits & var_mask_neg[var] ) | ( ( bits & var_mask_neg[var] ) << ( 1 << var ) ) );
}

inline Truth_Table Truth_Table::derivative( uint8_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) ^ negative_cofactor( var );
}

inline Truth_Table Truth_Table::consensus( uint8_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) & negative_cofactor( var );
}

inline Truth_Table Truth_Table::smoothing( uint8_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) | negative_cofactor( var );
}*/

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
  assert ( var < num_var );
  std::vector<uint64_t> nth_var = polarity ? var_mask_pos(var, num_var) : var_mask_neg(var, num_var);
  return Truth_Table( num_var, nth_var );
}