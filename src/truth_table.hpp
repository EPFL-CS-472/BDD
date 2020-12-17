#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>


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


std::vector<uint64_t> length_masks (uint32_t num_var){
	std::vector<uint64_t> mask ;
	int blocks ;
	uint64_t one = 0xffffffffffffffff ;

	if (num_var <7 ) 
	{
		blocks=1;
	}
	else
	{
		blocks= 1 << (num_var-6); 
	} 
	if (num_var <7) 
	{
		mask.emplace_back(length_mask[num_var]);
	}
	else
	{
		for (int j=0 ; j<blocks;j++)
		{ 
			mask.emplace_back(one);
		}
	} 
	return mask ;
} 


std::vector<uint64_t> msk_neg (uint32_t var,uint32_t num_var){
	std::vector<uint64_t> mask_var ;
	int blocks ;
	uint64_t one = 0xffffffffffffffff ;
	
	if (num_var <7 ) 
	{
		blocks=1;
	}
	else
	{
		blocks= 1 << (num_var-6);
	} 
	int position ;
	if (var <6 ) 
	{
		position = var;
	}
	else
	{
		position= var-6;
	} 
	int repetition = blocks / (1 << position); 
	if (var <6 ) 
	{
		for (int i=0 ; i<blocks;i++)
		{
			mask_var.emplace_back(var_mask_neg[var]); 	 
		}
	}
	else
	{
		for (int j=0 ; j<repetition;j++)
		{
			for (int k=0 ; k<(1<<position);k++)
			{
				mask_var.emplace_back(one);
			}
			one = ~one; 	 
		}
	}
	return mask_var;
}


std::vector<uint64_t> msk_pos (uint32_t var,uint32_t num_var){
	std::vector<uint64_t> mask_var ;
	int blocks ;
	uint64_t zero = 0x0000000000000000 ;
	
	if (num_var <7 ) 
	{
		blocks=1;
	}
	else
	{
		blocks= 1 << (num_var-6); 
	} 
	int position ; 
	if (var <6 ) 
	{
		position=var;
	}
	else
	{
		position= var-6;
	} 

	int repetition=blocks / (1 << position); 
	if (var <6 ) 
	{
		for (int i=0 ; i<blocks;i++)
		{
			mask_var.emplace_back(var_mask_pos[var]);
		}
	}
	else
	{
		for (int j=0 ; j<repetition;j++)
		{
			for (int k=0 ; k<(1<<position);k++)
			{
				mask_var.emplace_back(zero);
			}
			zero=~zero; 
		}
	} 
	return mask_var;
}
	

inline std::vector<uint64_t> and_vector( std::vector<uint64_t> vector1, std::vector<uint64_t> vector2 )
{
	std::vector<uint64_t> AND_V ;
	assert( vector1.size() == vector2.size());
	for (int i=0 ; i< vector1.size(); i++ )
	{
		AND_V.emplace_back (vector1.at(i) & vector2.at(i));
	}
	return AND_V;
}


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
    case 512u: return 9u;
    case 1024u: return 10u;
    default: return 0u;
  }
}


class Truth_Table
{
public:
  Truth_Table( uint32_t num_var )
  :num_var(num_var), bits()
  {
	  uint64_t tt_size= 1 << num_var;  // pow(2,num_var);
	  uint64_t blocks ;
      if (  tt_size % 64 == 0 ) 
      {
		  blocks = (tt_size / 64) ;
	  }
      else  
      {
		  blocks = (tt_size/ 64) +1 ;
	  }
      for(int i=0; i<blocks; i++)
      {
		  bits.push_back(0u);
      }
  }


  Truth_Table( uint32_t num_var, std::vector<uint64_t> bits )
   : num_var( num_var ), bits( and_vector (bits , length_masks(num_var) ))
  {
  }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits()
  {
	  uint64_t tt_size= 1 << num_var;  // pow(2,num_var);
      uint64_t blocks ;
      if (  tt_size % 64 == 0 )
      {
		  blocks = (tt_size / 64) ;
	  }
      else  
      {
		  blocks = (tt_size/ 64) +1 ;
	  }
      for(int i=0; i<blocks; i++)
      {
		  bits.push_back(0u);
      }
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
 

  bool get_bit( uint32_t const var_pos ) const
  {
    assert( var_pos < ( 1 << num_var ) );
    
    uint32_t div = var_pos / 64 ;
    uint32_t reste = var_pos % 64 ;
    
    return (( bits.at(div) >> reste ) & 0x1 );
  }

  void set_bit( uint32_t const var_pos )
  {
    assert( var_pos < ( 1 << num_var ) );
    uint32_t div = var_pos / 64 ;
    uint32_t reste = var_pos % 64 ;
   
    bits.at(div) |= ( uint64_t( 1 ) << reste );
    bits.at(div) &= length_masks(num_var).at(div);
  }

  uint32_t n_var() const
  {
    return num_var;
  }

  //Truth_Table positive_cofactor( uint8_t const var ) const;
  //Truth_Table negative_cofactor( uint8_t const var ) const;
  //Truth_Table derivative( uint8_t const var ) const;
  //Truth_Table consensus( uint8_t const var ) const;
  //Truth_Table smoothing( uint8_t const var ) const;

public:
  uint32_t const num_var; /* number of variables involved in the function */

  std::vector<uint64_t> bits; /* the truth table */
};


/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )
{
  for ( int32_t i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
  {
    os << ( tt.get_bit( i ) ? '1' : '0' );
  }
  return os;
}


/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
	std::vector<uint64_t> bits_interm ; 
	for (int i=0 ; i< tt.bits.size();i++)
	{
		bits_interm.emplace_back(~tt.bits.at(i));
	}
   return Truth_Table( tt.num_var, bits_interm );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
    assert( tt1.bits.size() == tt2.bits.size() );
    std::vector<uint64_t> bits_v ;
	for (int i=0 ; i< tt1.bits.size();i++)
	{
		bits_v.emplace_back(tt1.bits.at(i) | tt2.bits.at(i));
	}
    return Truth_Table( tt1.num_var, bits_v );
}


/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
    assert( tt1.bits.size() == tt2.bits.size() );
    std::vector<uint64_t> bits_v ;
	for (int i=0 ; i< tt1.bits.size();i++)
	{
		bits_v.emplace_back( tt1.bits.at(i) & tt2.bits.at(i) );
	}
    return Truth_Table( tt1.num_var, bits_v );
}


/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
    assert( tt1.bits.size() == tt2.bits.size() );
    std::vector<uint64_t> bits_v  ;
	for (int i=0 ; i< tt1.bits.size();i++)
	{
		bits_v.emplace_back(tt1.bits.at(i) ^ tt2.bits.at(i));
	}
    return Truth_Table( tt1.num_var, bits_v );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  bool verif ;
  if ( tt1.num_var != tt2.num_var )
  {
    return false;
  }
    for (int i=0 ; i<tt2.bits.size(); i++)
    {
		if (tt1.bits.at(i)!=tt2.bits.at(i))
		{
			return false ;
		}
	}
    return true ;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  return !( tt1 == tt2 );
}

inline Truth_Table create_tt_nth_var( uint32_t const num_var, uint32_t const var, bool const polarity = true )
{
  return Truth_Table( num_var, polarity ?  msk_pos(var, num_var) : msk_neg(var, num_var) );
}
