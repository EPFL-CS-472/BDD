#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
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
inline uint32_t power_two( const uint32_t n )
{
	uint32_t radix = 1;
	for( uint32_t i =0 ; i < n; i++){
	radix = radix*2;}
	return radix;
}

inline uint8_t ln_two( const uint32_t n )
{
	return log2(n);                        /* will be used to obtain number of variables*/
}
class Truth_Table
{
public:
  Truth_Table( uint8_t num_var )
   : num_var( num_var )
  {
    for(int i=0; i <= num_vc; i++){bits.push_back(0u);}          /* create vector of uint64_t with 0u at each member of the vector*/
  }

  Truth_Table( uint8_t num_var, std::vector<uint64_t>  bits )
   : num_var( num_var )
  {
    for(int i=0; i <= num_vc ; i++){this->bits.push_back(bits.at(i) &= length_mask[num_var%64]);}
  }

  Truth_Table( const std::string str )
   : num_var( ln_two( str.size() ) ), bits( 0u )
  {
	int L =(str.size()/64);
    if ( num_var == 0u )
    {
      return;
    }
	for(int i=0; i <= L; i++)  { bits.push_back(0u);}
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
	uint32_t vector_num = position/64;                     /* vector which contains member with position equal to position */
	uint32_t member_num = position%64;					/* position in the member  */
    return ( (  bits.at(vector_num) >> member_num ) & 0x1 );
  }
 

  void set_bit( uint8_t const position )
  { uint32_t vector_num = position/64;
	uint32_t member_num = position%64;
    bits.at(vector_num) |= ( uint64_t( 1 ) << member_num );
    bits.at(vector_num) &= length_mask[num_var%64];          /* to be able to use the provided mask I have opted to use length_mask.at( num_var modulo 64] */
  }

  uint8_t n_var() const
  {
    return num_var;
  }


public:
  uint8_t const num_var; /* number of variables involved in the function */
  std::vector<uint64_t>  bits; /* the truth table */
  uint32_t num_vc= (power_two(num_var)/64);
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
	std::vector<uint64_t>  vector_not;
	for(uint32_t i = 0; i< tt.bits.size() ; i++) {                   /* work on bits wich is vector of uint64_t to complement each member of the vector bits*/
		vector_not.emplace_back(~tt.bits.at(i));}
	return Truth_Table( tt.num_var, vector_not );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.bits.size() == tt2.bits.size() );
	std::vector<uint64_t>  vector_or;                             /* same as in not, or of each two members of the vectors are stored in the result vector vector_or*/
	for(uint32_t i = 0; i< tt1.bits.size() ; i++) {
		vector_or.emplace_back( tt1.bits.at(i) | tt2.bits.at(i) );}
	return Truth_Table( tt1.num_var, vector_or );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.bits.size() == tt2.bits.size() );
	std::vector<uint64_t>  vector_and;
	for(uint32_t i = 0; i< tt1.bits.size() ; i++) {
		vector_and.emplace_back( tt1.bits.at(i) & tt2.bits.at(i) );}
	return Truth_Table( tt1.num_var, vector_and );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.bits.size() == tt2.bits.size() );
	std::vector<uint64_t>  vector_xor;
	for(uint32_t i = 0; i< tt1.bits.size() ; i++) {
		vector_xor.emplace_back( tt1.bits.at(i) ^ tt2.bits.at(i) );}
	return Truth_Table( tt1.num_var, vector_xor );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	if ( tt1.bits.size() != tt2.bits.size() ){return false;}
	for(uint32_t i = 0; i< tt1.bits.size() ; i++) {
	if (tt1.bits.at(i) != tt2.bits.at(i)){
		return false;}
	}
	return true;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	return !( tt1 == tt2 );
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
	uint32_t vector_num = var/64;
	uint32_t member_num = var%64;
	std::vector<uint64_t>  bits;

	for(int i=0; i<(power_two(num_var)/64)+1; i++){
        if (polarity) { bits.push_back( var_mask_pos[member_num]);}						/* same structure as previous function given by the TA if polarity work with pos_mask else mask_neg*/
        else {bits.push_back( var_mask_neg[member_num]);}
	}
	return Truth_Table( num_var, bits);
}
