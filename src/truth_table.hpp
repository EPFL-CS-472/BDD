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


std::vector<uint64_t> length_mask_fct (uint32_t num_var){
	std::vector<uint64_t> length_mask_interm ;
	uint64_t fix = 0xffffffffffffffff ;
		/*initialisation */
	
	int nb_block ;
	if (num_var <7 ) {
		nb_block=1;
	}
		else{
		nb_block= 1 << (num_var-6); 
	} 
	if (num_var <7) {
		length_mask_interm.emplace_back(length_mask[num_var]);
	}
	else{
		for (int j=0 ; j<nb_block;j++){ 
			length_mask_interm.emplace_back(fix);
		
		}
	} 
	return length_mask_interm ;
} 


std::vector<uint64_t> generate_msk_pos (uint32_t var,uint32_t num_var){
	std::vector<uint64_t> mask_var ;
	uint64_t fix = 0x0000000000000000 ;
	//std::cout << "entered generate_msk_pos " << std::endl;
	int nb_block ;
	if (num_var <7 ) {
		nb_block=1;
	}
	else{
		nb_block= 1 << (num_var-6); //power_two(num_var-6);
	} 
 //   std::cout << "nb_block " <<  nb_block <<std::endl;
	int pos ; //pos= var-6;
	if (var <6 ) {
		pos=var;
	}
	else{
		pos= var-6;
	} 
	//std::cout << "pos " << pos <<std::endl;
	
	int unite=nb_block / (1 << pos); //power_two(pos); /* a revoir si -1 ou pas */
	//std::cout << "unite " << unite <<std::endl;
	//std::cout << "var" <<var <<std::endl;
	if (var <6 ) {
		for (int i=0 ; i<nb_block;i++){
			//std::cout << "entered boucle " << var <<" i " << i <<std::endl;
			mask_var.emplace_back(var_mask_pos[var]);
			//std::cout << "mask " <<   var_mask_pos[var] <<std::endl;
			//std::cout << "mask_var " << mask_var.at(i)   <<std::endl; 
		}
	}
	else{
	//	std::cout << "entered boucle ELSE " <<std::endl;
		for (int j=0 ; j<unite;j++){ 
			//std::cout << "entered boucle j " << var <<" j " << j <<std::endl;
			for (int k=0 ; k<(1<<pos);k++){
				//std::cout << "entered boucle k " << var <<" k " << k <<std::endl;
				mask_var.emplace_back(fix);
			    //std::cout << "mask_var " << mask_var   <<std::endl;
			}
			
			fix=~fix; 
			
			 
		}
	} 
		
//	std::cout << "sortie generate_msk_pos " << std::endl;
	return mask_var;
}
	
	

std::vector<uint64_t> generate_msk_neg (uint32_t var,uint32_t num_var){
	std::vector<uint64_t> mask_var ;
	uint64_t fix = 0xffffffffffffffff ;
	
	//std::cout << "entered generate_msk_neg " << std::endl;
	//std::cout << "num_var " << num_var  <<std::endl; 
	int nb_block ;
	if (num_var <7 ) {
		nb_block=1;
	}
		else{
		nb_block= 1 << (num_var-6);
	} 
  
	int pos ; 
	if (var <6 ) {
		pos=var;
		}
	else{
		pos= var-6;
	} 
	//std::cout << "pos " << pos <<std::endl;
	
	int unite=nb_block / (1 << pos); 
	
		
		///*initialisation */
	
	if (var <6 ) {
		for (int i=0 ; i<nb_block;i++){
			//std::cout << "entered boucle " << var <<" i " << i <<std::endl;
			mask_var.emplace_back(var_mask_neg[var]); 
			//std::cout << "mask " <<   var_mask_neg[var] <<std::endl;

		}
	}
	else{
		//std::cout << "entered boucle ELSE " <<std::endl;
		for (int j=0 ; j<unite;j++){ 
			//std::cout << "entered boucle j " << var <<" j " << j <<std::endl;
			for (int k=0 ; k<(1<<pos);k++){
				
				mask_var.emplace_back(fix);
			    
			}
			
			fix=~fix; 
			
			 
		}
	}
	
	//std::cout << "sortie generate_msk_pos " << std::endl;
	return mask_var;
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
    case 512u: return 9u;
    case 1024u: return 10u;
     default: return 0u;
  }
}



/*this function returns the result of AND for a vector of uint64*/
inline std::vector<uint64_t> op_and( std::vector<uint64_t> bits1, std::vector<uint64_t> bits2 )
{
	std::vector<uint64_t> res ;
  assert( bits1.size() == bits2.size());
  
  for (int i=0 ; i< bits1.size(); i++ ){
	res.emplace_back (bits1.at(i) & bits2.at(i));
	}
  return res;
}


class Truth_Table
{
public:
  Truth_Table( uint32_t num_var )
  :num_var(num_var), bits()
  {
        uint64_t f_size= 1 << num_var;  // pow(2,num_var);
        uint64_t blocks ;
        if (  f_size % 64 == 0 ) {
        blocks = (f_size / 64) ;}
        else  {blocks = (f_size/ 64) +1 ;}
        for(int i=0; i<blocks; i++){
            bits.push_back(0u);
        }
    }


  Truth_Table( uint32_t num_var, std::vector<uint64_t> bits )
   : num_var( num_var ), bits( op_and (bits , length_mask_fct(num_var) ))
  {
   
  }



  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits()
  { 
	  uint64_t f_size= 1 << num_var;  
        uint64_t blocks ;
        if (  f_size % 64 == 0 ) {
        blocks = (f_size / 64) ;}
        else  {blocks = (f_size/ 64) +1 ;}
        for(int i=0; i<blocks; i++){
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
    //std::cout << "blocks" << blocks << std::endl;
    //std::cout << "num var" << (uint32_t)num_var << std::endl;
  }
 


  bool get_bit( uint32_t const position ) const
  {
    assert( position < ( 1 << num_var ) );
    
    uint32_t dividende = position / 64 ;
    uint32_t reste = position % 64 ;
    
    return (( bits.at(dividende) >> reste ) & 0x1 );
  }





  void set_bit( uint32_t const position )
  {
    assert( position < ( 1 << num_var ) );
    uint32_t dividende = position / 64 ;
    uint32_t reste = position % 64 ;
   
    bits.at(dividende) |= ( uint64_t( 1 ) << reste );
    bits.at(dividende) &= length_mask_fct(num_var).at(dividende);
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
	for (int i=0 ; i< tt.bits.size();i++){
		bits_interm.emplace_back(~tt.bits.at(i));
		}
		
  return Truth_Table( tt.num_var, bits_interm );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
   assert( tt1.bits.size() == tt2.bits.size() );
  std::vector<uint64_t> bits_interm ;
	for (int i=0 ; i< tt1.bits.size();i++){
		bits_interm.emplace_back(tt1.bits.at(i) | tt2.bits.at(i));
		}
  return Truth_Table( tt1.num_var, bits_interm );
}

/* bit-wise AND operation */

inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
    assert( tt1.bits.size() == tt2.bits.size() );
    std::vector<uint64_t> bits_interm ;
	for (int i=0 ; i< tt1.bits.size();i++){
		bits_interm.emplace_back( tt1.bits.at(i) & tt2.bits.at(i) );
	}
    return Truth_Table( tt1.num_var, bits_interm );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );
    assert( tt1.bits.size() == tt2.bits.size() );
  std::vector<uint64_t> bits_interm  ;
	for (int i=0 ; i< tt1.bits.size();i++){
		bits_interm.emplace_back(tt1.bits.at(i) ^ tt2.bits.at(i));
		}
  return Truth_Table( tt1.num_var, bits_interm );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  bool verif ;
  if ( tt1.num_var != tt2.num_var )
  {
    return false;
  }
    for (int i=0 ; i<tt2.bits.size(); i++){
		if (tt1.bits.at(i)!=tt2.bits.at(i)){
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
  return Truth_Table( num_var, polarity ?  generate_msk_pos(var, num_var) : generate_msk_neg(var, num_var) );
}


