#pragma once

#include <iostream>
#include <cassert>
#include <vector>
#include <string>

/* helper function for power calculation */
inline int power(int val, int pow){ //OK
  if(pow == 0) return 1;
  int ret = val;
  for(int i=0; i<pow-1;i++) ret *= val;
  return ret;
}

/* masks used to filter out unused bits */
inline uint64_t length_mask(uint32_t num_var){ //OK
  assert(num_var <= 6);
  uint64_t mask = 1;
  while(num_var>=1){
  	for(int i=0; i<power(2, num_var-1); i++){
  		mask = mask << 1;
  		mask |= 0x1;
  	}
  	num_var--;
  }
  return mask;
}

/* return i if n == 2^i, 0 otherwise */
inline uint32_t power_two( const uint32_t n )	//OK
{
  if(n == 0) return 0;
  uint32_t div = n;
  uint32_t i = 0;
  while(div!=1){
  	if(div%2 != 0) return 0;
  	div /= 2;
  	i++;
  }
  return i;
}

class Truth_Table
{
public:
  Truth_Table( uint32_t num_var )	//OK
   : num_var( num_var ), bits( (num_var <6) ? 1u : (1u << (num_var - 6)) )
  {

  }
  
  Truth_Table( uint32_t num_var, std::vector<uint64_t> _bits )	//OK
   : num_var( num_var ), bits( _bits )
  {
	size_t blocks = (num_var <6) ? 1u : (1u << (num_var - 6));
	bits.resize(blocks);
	if(num_var < 6) bits[0u] &= length_mask(num_var);
  }
  
  Truth_Table( const std::string str )	//OK //WARNING strings have to be of length power 2
   : num_var( power_two( str.size() ) ), bits( (power_two( str.size() ) < 6) ? 1u : (1u << (power_two( str.size() ) - 6)) )
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

  bool get_bit( uint32_t const position ) const //OK 
  {
    assert( position < ( 1 << num_var ) );
    uint32_t block = position/64; //Compute the relevant block index
    uint32_t block_position = position % 64;	//Compute new position in block
    return ( ( bits[block] >> block_position ) & 0x1 );
  }

  void set_bit( uint32_t const position )	//OK 
  {
    assert( position < ( 1 << num_var ) );
    uint32_t block = position/64; //Compute the relevant block index
    uint32_t block_position = position % 64;	//Compute new position in block
    bits[block] |= ( uint64_t( 1 ) << block_position );
    if(num_var < 6) bits[0u] &= length_mask(num_var);
  }

  uint32_t n_var() const	//OK
  {
    return num_var;
  }
  
  Truth_Table positive_cofactor( uint32_t const var ) const;
  Truth_Table negative_cofactor( uint32_t const var ) const;
  Truth_Table derivative( uint32_t const var ) const;
  Truth_Table consensus( uint32_t const var ) const;
  Truth_Table smoothing( uint32_t const var ) const;

public:
  uint32_t const num_var; /* number of variables involved in the function */
  std::vector<uint64_t> bits; /* the truth table */
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )	//OK
{
  for ( auto i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
  {
    os << ( tt.get_bit( i ) ? '1' : '0' );
  }
  return os;
}

inline Truth_Table operator<<(Truth_Table const& tt1, int shift){	//OK
  std::vector<uint64_t> vec (tt1.bits);
  uint64_t save_next;
  uint64_t save = 0x0;
  for(auto i=0; i<tt1.bits.size(); i++){
  	save_next = vec[i];
  	vec[i] = vec[i] << shift;
  	vec[i] |= (save >> (64-shift));
  	save = save_next;
  }
  return Truth_Table(tt1.num_var, vec);
}

inline Truth_Table operator>>(Truth_Table const& tt1, int shift){	//OK
  std::vector<uint64_t> vec (tt1.bits);
  uint64_t save_next;
  uint64_t save = 0x0;
  for(auto i=tt1.bits.size()-1; i>=0; i--){
  	save_next = vec[i];
  	vec[i] = vec[i] >> shift;
  	vec[i] |= (save << (64-shift));
  	save = save_next;
  }
  return Truth_Table(tt1.num_var, vec);
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )	//OK
{
  std::vector<uint64_t> inv_bits (tt.bits);
  for(auto i=0; i<inv_bits.size(); i++) inv_bits[i] = ~inv_bits[i];
  return Truth_Table( tt.num_var, inv_bits );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )	//OK
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> or_bits (tt1.bits);
  for(auto i=0; i<or_bits.size(); i++) or_bits[i] |= tt2.bits[i];
  return Truth_Table( tt1.num_var, or_bits );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )	//OK
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> and_bits (tt1.bits);
  for(auto i=0; i<and_bits.size(); i++) and_bits[i] &= tt2.bits[i];
  return Truth_Table( tt1.num_var, and_bits );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )	//OK
{
  assert( tt1.num_var == tt2.num_var );
  std::vector<uint64_t> xor_bits (tt1.bits);
  for(auto i=0; i<xor_bits.size(); i++) xor_bits[i] = xor_bits[i]^tt2.bits[i];
  return Truth_Table( tt1.num_var, xor_bits);
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )	//OK
{
  if ( tt1.num_var != tt2.num_var )
  {
    return false;
  }
  bool ret = true;
  for(auto i=0; i<tt1.bits.size(); i++) ret &= (tt1.bits[i] == tt2.bits[i]);
  return ret;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )	//OK
{
  return !( tt1 == tt2 );
}

/* masks used to get the bits where a certain variable is 1 */  
inline Truth_Table var_mask_pos(uint32_t num_var, uint32_t var){ //OK
  assert(var < num_var);
  Truth_Table tt = Truth_Table(num_var);
  if(var < 6){
  	int batch_size = power(2, var);
  	for(auto i=0; i<tt.bits.size(); i++){ //Block iteration
  		for(auto j=0; j<64; j++){ //Bit iteration
  		  	tt.bits[i] = tt.bits[i] >> 1;
  			tt.bits[i] |= (((j / batch_size) % 2) == 0) ? 0x0 : 0x8000000000000000;

  		}
  	}
  }else{
  	int batch_size = power(2, var-6);
  	for(auto i=0; i<tt.bits.size(); i++){ //Block iteration
  		tt.bits[i] = (((i / batch_size) % 2) == 0) ? 0x0 : 0xffffffffffffffff;
  	}
  }
  return tt;
}

/* masks used to get the bits where a certain variable is 0 */
inline Truth_Table var_mask_neg(uint32_t num_var, uint32_t var){ //OK
  assert(var < num_var);
  Truth_Table tt = Truth_Table(num_var);
  if(var < 6){
  	int batch_size = power(2, var);
  	for(auto i=0; i<tt.bits.size(); i++){ //Block iteration
  		for(auto j=0; j<64; j++){ //Bit iteration
  		  	tt.bits[i] = tt.bits[i] >> 1;
  			tt.bits[i] |= (((j / batch_size) % 2) == 0) ? 0x8000000000000000 : 0x0;

  		}
  	}
  }else{
  	int batch_size = power(2, var-6);
  	for(auto i=0; i<tt.bits.size(); i++){ //Block iteration
  		tt.bits[i] = (((i / batch_size) % 2) == 0) ? 0xffffffffffffffff : 0x0;
  	}
  }
  return tt;
}

inline Truth_Table Truth_Table::positive_cofactor( uint32_t const var ) const
{
  assert( var < num_var );
  Truth_Table pos = var_mask_pos(num_var, var);
  return (*this & pos) | ((*this & pos) >> (1 << var));
}

inline Truth_Table Truth_Table::negative_cofactor( uint32_t const var ) const
{
  assert( var < num_var );
  Truth_Table neg = var_mask_neg(num_var, var);
  return (*this & neg) | ((*this & neg) << (1 << var));
}

inline Truth_Table Truth_Table::derivative( uint32_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) ^ negative_cofactor( var );
}

inline Truth_Table Truth_Table::consensus( uint32_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) & negative_cofactor( var );
}

inline Truth_Table Truth_Table::smoothing( uint32_t const var ) const
{
  assert( var < num_var );
  return positive_cofactor( var ) | negative_cofactor( var );
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint32_t const num_var, uint32_t const var, bool const polarity = true )
{
  assert ( var < num_var ); 
  return Truth_Table( num_var, polarity ? var_mask_pos(num_var, var).bits : var_mask_neg(num_var, var).bits );
}

