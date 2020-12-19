#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <bitset> // DEBUG

static const uint64_t full = 0xffffffffffffffff; // used for more compact notation

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

/* Student comment
* ---------------
* This functions is used to get the mask with all position corresponding to "var" set to 1
*
* It is defined on top of the existing constants above to extends those masks to vectors of uint64_t
* (i.e. TT that have more than 6 variables).
*
* Takes : numvar -> number of variables of the function, var -> variable to mask positively
*
* Returns : vector of uint64_t with all bits set when var = 1
*/
inline std::vector<uint64_t> get_var_pos(uint8_t numvar, uint8_t var){
    assert(var < numvar);
    /* Student comment
    * ---------------
    * If the variable is < 6, its masks is one of the predefined constants,
    * which must be placed in all elements of the TT.
    */
    std::vector<uint64_t> ret;
    if(var < 6){
        /* Student comment
        * ---------------
        * If numvar is < 6, the function never enters the loop, so we add a first entry
        * and then start the loop at index 1.
        */
        ret.push_back(var_mask_pos[var]);
        /* Student comment
        * ---------------
        * For a number of variable > 6, we must have 2^^(numvar-6) elements in the TT vector.
        */
        for(size_t i = 1; i < (1 << (numvar - 6)) && numvar > 6 ; i++)
            ret.push_back(var_mask_pos[var]);
    }else{
        /* Student comment
        * ---------------
        * If the variable to mask if of order > 6, the masks is composed of uint64_t either completely = 0
        * or completely = 1.
        *
        * For a variable of order n, we have 2^^n 0s, then 2^^n 1s. As an entry is composed of 64 = 2^^6 bits,
        * we need 2^^(n-6) 0s followed by 2^^(n-6) 1s repeated until we have filled all the bits.
        */
        uint8_t n = 1 << (var - 6);
        for(size_t i = n; i < numvar - 1; i++){
            for(size_t j = 0; j < n; j++)
                ret.push_back(0x0);
            for(size_t j = 0; j < n; j++)
                ret.push_back(full);
        }

    }
    /* Student comment
    * ---------------
    * If the number of variable is < 6, we need to apply a mask to discard unused bits.
    */
    if(numvar < 6)
        ret.at(0) &= length_mask[numvar];

    return ret;
}

/* Student comment
* ---------------
* This functions is used to get the mask with all position corresponding to "~var" set to 1
*
* It is defined on top of the existing constants above to extends those masks to vectors of uint64_t
* (i.e. TT that have more than 6 variables).
*
* Takes : numvar -> number of variables of the function, var -> variable to mask negatively
*
* Returns : vector of uint64_t with all bits set when var = 0
*/
inline std::vector<uint64_t> get_var_neg(uint8_t numvar, uint8_t var){
    assert(var < numvar);

    std::vector<uint64_t> ret;
    if(var < 6){
        ret.push_back(var_mask_neg[var]);
        for(size_t i = 1; i < (1 << (numvar - 6)) && numvar > 6; i++)
            ret.push_back(var_mask_neg[var]);
    }else{
        uint8_t n = 1 << (var - 6);
        for(size_t i = n; i < numvar - 1; i++){
            for(size_t j = 0; j < n; j++)
                ret.push_back(full);
            for(size_t j = 0; j < n; j++)
                ret.push_back(0x0);
        }
    }

    if(numvar < 6)
        ret.at(0) &= length_mask[numvar];

    return ret;
}

/* Student comment
* ---------------
* This functions shifts the provided TT to the right.
*
* First, it shifts by chunks of 64bits, allowed by the division of the TT in a vector of 64-bits elements
* Then, it shifts the remaining number of from the right (LSB) to the left.
* To properly transpose the LSB from the leftmost neighbor in the vector, we shift it to the left to keep the desired bits
* and we compute the union of the rightmost neighbor after shifting it to the right.
*
* E.G. : if TT = 01011100011, bits = {0111, 1110, 0011}, with pos = 2
*
*           ret = {0001, (1100 | 0011), (1000 | 0000)}
*
* Takes : bits -> pointer to the TT as a vector of uint64_t, pos -> the number of position to shift
*
* Returns : nothing, the TT is modified internally
*/
inline void shift_right_vec(std::vector<uint64_t>* bits, uint8_t pos){
    uint64_t temp;
    uint8_t mat_shift = pos >> 6;
    size_t i;
    // Shift entire elements if number of pos to shift is > 64
    for(i = 0; i < bits->size() - mat_shift && mat_shift > 0; i++){
        bits->at(i) = bits->at(i+mat_shift);
    }
    // Set remaining element to 0
    for(; i < bits->size(); i++)
        bits->at(i) = 0u;
    // Shift remaining pos if needed
    uint8_t remaining_pos = pos - (64 << mat_shift);
    for(i = 0; i < bits->size() - 1 && remaining_pos; i++){
        temp = bits->at(i+1) << (64 - remaining_pos);
        bits->at(i) = temp | (bits->at(i) >> remaining_pos);
    }
    // Shift last element
    bits->at(i) = bits->at(i) >> remaining_pos;
}

/* Student comment
* ---------------
* This functions shifts the provided TT to the left.
*
* First, it shifts by chunks of 64bits.
* Then, it shifts the remaining number of from the left (MSB) to the right.
* To properly transpose the LSB from the rightmost neighbour in the vector, we shift it to the right to keep the desired bits
* and we compute the union of the leftmost neighbor after shifting it to the left.
*
* E.G. : if TT = 01011100011, bits = {0111, 1110, 0011}, with pos = 2
*
*           ret = {(0100 | 0011), (1000 | 0000), 1100}
*
* Takes : bits -> pointer to the TT as a vector of uint64_t, pos -> the number of position to shift
*
* Returns : nothing, the TT is modified internally
*/
inline void shift_left_vec(std::vector<uint64_t>* bits, uint8_t pos){
    uint64_t temp;
    uint8_t mat_shift = pos >> 6;
    size_t i;
    // Shift entire element if number of pos to shift is > 64
    for(i = bits->size() - 1; i >= mat_shift && mat_shift > 0; i--){
        bits->at(i) = bits->at(i - mat_shift);
    }
    // Set remaining element to 0
    for(size_t j = 0; j < i + 1; j++)
        bits->at(j) = 0u;
    // Shift remaining pos if needed
    uint8_t remaining_pos = pos - (64 << mat_shift);
    for(i = bits->size() - 1; i > 0  && remaining_pos; i--){
        temp = bits->at(i-1) >> (64 - remaining_pos);
        bits->at(i) = temp | (bits->at(i) << remaining_pos);
    }
    // Shift last element
    bits->at(i) = bits->at(i) << remaining_pos;
}

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
    default: return 0u;
  }
}

/* Student comment
* ---------------
* This functions compute the floored log2 of the number provided
*
* Takes : unsigned number
*
* Returns : closest integer a s.t. a < log2(n)
*/
inline uint8_t log_two( const uint8_t n )
{
  uint8_t ret = 0;

  while((n >> 1) > 0)
      ret++;

  return ret;
}

/* Student comment
* ---------------
* This functions builds a vector of 0s representing the TT
*
* Takes : the number of variable in the TT
*
* Returns : a vector of uin64_t representing the TT
*/
inline std::vector<uint64_t> build_bits(uint8_t numvars){
    std::vector<uint64_t> ret;
    ret.push_back(0u);
    if(numvars > 6)
    {
        uint32_t num_table = 1 << (numvars - 6);
        for(size_t i = 1; i < num_table; i++){
            ret.push_back(0u);
        }
    }
    return ret;
}

class Truth_Table
{
public:
  using bits_t = std::vector<uint64_t>;

  Truth_Table( uint8_t num_var )
      : num_var( num_var ), bits( build_bits(num_var) )
  {
  }

  /* Student comment
  * ---------------
  * This is a TT constructor to which a vector of uint64_t can be passed
  */
  Truth_Table( uint8_t num_var, bits_t bits_v )
      : num_var( num_var ), bits( bits_v )
  {
  }

  Truth_Table( uint8_t num_var, uint64_t bits )
   : num_var( num_var ), bits( {bits & length_mask[num_var]} )
  {
    assert( num_var <= 6u );
  }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits( build_bits(power_two( str.size() ) ))
  {
    if ( num_var == 0u )
    {
      return;
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

  bool get_bit( uint8_t const position ) const
  {
    assert( position < ( 1 << num_var ) );
    // If the position is > 64, we need first to find in which entry of the vector it is
    // then compute its offset in the corresponding entry
    uint8_t index = position >> 6u;
    return ( ( bits.at(index) >> (position - (index*64))) & 0x1 );
  }

  void set_bit( uint8_t const position )
  {
    assert( position < ( 1 << num_var ) );
    // If the position is > 64, we need first to find in which entry of the vector it is
    // then compute its offset in the corresponding entry
    uint8_t index = position >> 6u;
    bits.at(index) |= ( uint64_t( 1 ) << (position - (index*64)) );
  }

  uint8_t n_var() const
  {
    return num_var;
  }

  Truth_Table positive_cofactor( uint8_t const var ) const;
  Truth_Table negative_cofactor( uint8_t const var ) const;
  Truth_Table derivative( uint8_t const var ) const;
  Truth_Table consensus( uint8_t const var ) const;
  Truth_Table smoothing( uint8_t const var ) const;

public:
  uint8_t const num_var; /* number of variables involved in the function */
  std::vector<uint64_t> bits; /* the truth table */
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
  std::vector<uint64_t> ret;
  // First, we invert all the bits in all the pieces of the TT
  for(size_t i = 0; i < tt.bits.size(); i++){
      ret.push_back(~(tt.bits.at(i)));
  }
  // Then, if the TT is less than 64 bits long, we must apply the appropriate mask to discard unused bits
  if(tt.num_var < 6)
      ret.at(0) &= length_mask[tt.num_var];
  return Truth_Table( tt.num_var, ret );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> ret;
  // We construct a new TT by apply the operation to all the chunks of the TTs
  for(size_t i = 0; i < tt1.bits.size(); i++){
      ret.push_back(tt1.bits.at(i) | tt2.bits.at(i));
  }
  return Truth_Table( tt1.num_var, ret );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> ret;
  // We construct a new TT by apply the operation to all the chunks of the TTs
  for(size_t i = 0; i < tt1.bits.size(); i++){
      ret.push_back(tt1.bits.at(i) & tt2.bits.at(i));
  }

  return Truth_Table( tt1.num_var, ret );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> ret;
  // We construct a new TT by apply the operation to all the chunks of the TTs
  for(size_t i = 0; i < tt1.bits.size(); i++){
      ret.push_back(tt1.bits.at(i) ^ tt2.bits.at(i));
  }

  return Truth_Table( tt1.num_var, ret );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  if ( tt1.num_var != tt2.num_var )
  {
    return false;
  }
  bool is_equal = true;
  for(size_t i = 0; i < tt1.bits.size(); i++){
      // We check that all chunks of the TTs are equal
        is_equal &= (tt1.bits.at(i) == tt2.bits.at(i));
  }
  return is_equal;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return !( tt1 == tt2 );
}

inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  bits_t ret, masked, shifted;
  // First, get the mask for the whole TT
  bits_t mask = get_var_pos(num_var, var);
  for(size_t i = 0; i < masked.size(); i++){
      // Compute the masked TT
      masked.push_back( bits.at(i) & mask.at(i));
      // Prepare the masked TT to be shifted in the next step
      shifted.push_back( bits.at(i) & mask.at(i));
  }
  // Compute the shifted masked TT
  shift_right_vec(&shifted, (1 << var));
  // For every chunk of the TT, compute the union of both variables
  for(size_t i = 0; i < bits.size(); i++){
      ret.push_back(masked.at(i) | shifted.at(i));
  }

  return Truth_Table( num_var, ret );
}

inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  bits_t ret, masked, shifted;
  // First, get the mask for the whole TT
  bits_t mask = get_var_neg(num_var, var);
  for(size_t i = 0; i < masked.size(); i++){
      // Compute the masked TT
      masked.push_back( bits.at(i) & mask.at(i));
      // Prepare the masked TT to be shifted in the next step
      shifted.push_back( bits.at(i) & mask.at(i));
  }

  // Compute the shifted masked TT
  shift_left_vec(&shifted, (1 << var));

  for(size_t i = 0; i < bits.size(); i++){
      // For every chunk of the TT, compute the union of both variables
      ret.push_back(masked.at(i) | shifted.at(i));
  }
  return Truth_Table( num_var, ret );
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
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
  assert (var < num_var );
  return Truth_Table( num_var, polarity ? get_var_pos(num_var, var) : get_var_neg(num_var, var) );
}
