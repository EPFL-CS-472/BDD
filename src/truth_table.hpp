#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>


/*
 * 
 * 
 * our data structure is built in a way such that the uint64_t is replaced by a vector of uint64_t as we want to manage 
 * more than 6 varables and our  processor is not able to manage more that 64 bit at a time.
 * data is stored form the least significant uint64 to the most signficant one
 * 
 * 
 */


/* bit-wise NOT operation */


std::vector<uint64_t> NOT_VECTOR( std::vector<uint64_t> opa ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opa.size(); i++){
		result.emplace_back(~opa.at(i));
		}
	return result;
	}

/* bit-wise OR operation */


std::vector<uint64_t> OR_VECTOR( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	//std::cout << "OR opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
	//assert( opa.size() == opb.size() );
	std::vector<uint64_t> result;
	if (opa.size() != opb.size() ){
		std::cout << "OR opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
		assert( opa.size() == opb.size() );
	}

	
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) | opb.at(i));
		}
	return result;
	}

/* bit-wise AND operation */


std::vector<uint64_t> AND_VECTOR( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	//std::cout << "AND opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
	if (opa.size() != opb.size() ){
		std::cout << "AND opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
		assert( opa.size() == opb.size() );
	}
	assert( opa.size() == opb.size() );

	std::vector<uint64_t> result;
	if ( opa.size() != opb.size() ){
		result.emplace_back(0);
		return result;
	}
		
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) & opb.at(i));
		}
	return result;
	}
	

/* bit-wise XOR operation */

std::vector<uint64_t> XOR_VECTOR( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	if (opa.size() != opb.size() ){
		std::cout << "XOR opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
		assert( opa.size() == opb.size() );
	}
	assert( opa.size() == opb.size() );
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) ^ opb.at(i));
		}
	return result;
	}

/* check if two truth_tables are the same */


bool IS_EQUAL_VECTOR ( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	if (opa.size() != opb.size() ){
		std::cout << "IS_equal opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
		assert( opa.size() == opb.size() );
	}
	assert( opa.size() == opb.size() );
	for (unsigned i=0; i<opb.size(); i++){
		if (opa.at(i) != opb.at(i))
		return false;
		}
	return true;
	}



bool IS_DIFFERENT_VECTOR ( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	if (opa.size() != opb.size() ){
		std::cout << "IS_DIFF opa and opb sizes " << opa.size() << " " << opb.size() << std::endl;
		assert( opa.size() == opb.size() );
	}
	assert( opa.size() == opb.size() );
	for (unsigned i=0; i<opb.size(); i++){
		if (opa.at(i) != opb.at(i))
		return true;
		}
	return false;
	}
	
std::vector<uint64_t> R_SHIFT_VECTOR ( std::vector<uint64_t> opa, uint8_t shift ){ 
	std::vector<uint64_t> result;
	
	for (unsigned i=0; i<opa.size(); i++){
		if (i + 1 < opa.size() ){ // this is not the first element of the vector, we have to use the previous one
			result.emplace_back( (opa.at(i) >> shift) | opa.at(i)<< (64-shift));
		}
		else {
			result.emplace_back (opa.at(i) >> shift);
		}
	}
	return result;
}

std::vector<uint64_t> L_SHIFT_VECTOR ( std::vector<uint64_t> opa, uint8_t shift ){ 
	std::vector<uint64_t> result;
	
	for (unsigned i=0; i<opa.size(); i++){
		if (i + 1 < opa.size() ){ // this is not the first element of the vector, we have to use the previous one
			result.emplace_back( (opa.at(i) << shift) | opa.at(i) >> (64-shift));
		}
		else {
			result.emplace_back (opa.at(i) << shift);
		}
	}
	return result;
}
	
	
	
/* Here we will generate masks with the appropriate length on vectors */	

/* masks used to filter out unused bits */
std::vector<uint64_t> length_mask (uint8_t num_var){
	std::vector<uint64_t> table;
	if (num_var==0){
		table.emplace_back(0x0000000000000001);
	}
	
	else if (num_var==1){
		table.emplace_back(0x0000000000000003);
	}
	
	else if (num_var==2){
		table.emplace_back(0x000000000000000f);
	}
		
	else if (num_var==3){
		table.emplace_back(0x00000000000000ff);
	}

	else if (num_var==4){
		table.emplace_back(0x000000000000ffff);
	}
		
	else if (num_var==5){
		table.emplace_back(0x00000000ffffffff);
	}
		
	else if (num_var==6){
		table.emplace_back(0xffffffffffffffff);
	}
		
	else { // we have more than 6 variables we return a table of 2^(num_var-6)
		for (uint8_t i = 0; i < (1 << (num_var-6)); i++){ //for the rigth amout of uint64_t in vector
			table.emplace_back(0xffffffffffffffff); 
		}
	}
	return table;
}
	
	
	

/* masks used to get the bits where a certain variable is 1 */
std::vector<uint64_t> var_mask_pos(uint8_t var, uint8_t num_var){
	
	std::vector<uint64_t> table;
	
	if (var==0){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xaaaaaaaaaaaaaaaa);
			}
		}
		else {
			table.emplace_back(0xaaaaaaaaaaaaaaaa);
		}
	}

	else if (var==1){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xcccccccccccccccc);
			}
		}
		else {
			table.emplace_back(0xcccccccccccccccc);
		}
	}
	
	else if (var==2){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xf0f0f0f0f0f0f0f0);
			}
		}
		else {
			table.emplace_back(0xf0f0f0f0f0f0f0f0);
		}
	}

	else if (var==3){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xff00ff00ff00ff00);
			}
		}
		else {
			table.emplace_back(0xff00ff00ff00ff00);
		}
	}

	else if (var==4){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xffff0000ffff0000);
			}
		}
		else {
			table.emplace_back(0xffff0000ffff0000);
		}
	}

	else if (var==5){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0xffffffff00000000);
			}
		}
		else {
			table.emplace_back(0xffffffff00000000);
		}
	}

	else { // we have more than 6 variables we return a table of 2^(num_var-6)
		for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
			if (i%(2*var-10) < (var-6)*(var-5)) {  //the idea there is to put 1 for num_var - 6 and then 0 for num_var - 6 until we reach the wanted size.
				table.emplace_back(0xffffffffffffffff); 
			}
			else {
				table.emplace_back(0x0000000000000000);
			}
		}
	}
	return table;
}

/*masks used to get the bits where a certain variable is 0 */
std::vector<uint64_t> var_mask_neg(uint8_t var, uint8_t num_var){
	std::vector<uint64_t> table;
	if (var==0){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x5555555555555555);
			}
		}
		else {
			table.emplace_back(0x5555555555555555);
		}
	}

	else if (var==1){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x3333333333333333);
			}
		}
		else {
			table.emplace_back(0x3333333333333333);
		}
	}
	
	else if (var==2){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x0f0f0f0f0f0f0f0f);
			}
		}
		else {
			table.emplace_back(0x0f0f0f0f0f0f0f0f);
		}
	}

	else if (var==3){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x00ff00ff00ff00ff);
			}
		}
		else {
			table.emplace_back(0x00ff00ff00ff00ff);
		}
	}

	else if (var==4){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x0000ffff0000ffff);
			}
		}
		else {
			table.emplace_back(0x0000ffff0000ffff);
		}
	}

	else if (var==5){
		if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				table.emplace_back(0x00000000ffffffff);
			}
		}
		else {
			table.emplace_back(0x00000000ffffffff);
		}
	}

	else { // we have more than 6 variables we return a table of 2^(num_var-6)
		for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
			if (i%(2*var-10) < (var-5)*(var-5)) {  //the idea there is to put 1 for num_var - 6 and then 0 for num_var - 6 until we reach the wanted size.
				table.emplace_back(0x0000000000000000); 
			}
			else {
				table.emplace_back(0xffffffffffffffff);
			}
		}
	}
	return table;
}



/*
static const uint64_t length_mask[] = {
  0x0000000000000001,   // LSB
  0x0000000000000003,   // 1/32 LSB
  0x000000000000000f,   // 1/16 LSB
  0x00000000000000ff,   // 1/8 LSB
  0x000000000000ffff,   // 1/4 LSB
  0x00000000ffffffff,   // 1/2 LSB
  0xffffffffffffffff};  // all bits
*/
/* 
static const uint64_t var_mask_pos[] = {
  0xaaaaaaaaaaaaaaaa,  // when var0 is 1 
  0xcccccccccccccccc,  // when var1 is 1 
  0xf0f0f0f0f0f0f0f0,  // when var2 is 1 
  0xff00ff00ff00ff00,  // when var3 is 1 
  0xffff0000ffff0000,  // when var4 is 1 
  0xffffffff00000000}; // when var5 is 1 
*/
/* 
static const uint64_t var_mask_neg[] = {
  0x5555555555555555,  // when var0 is at 0
  0x3333333333333333,  // when var1 is at 0
  0x0f0f0f0f0f0f0f0f,  // when var3 is at 0
  0x00ff00ff00ff00ff,  // when var3 is at 0
  0x0000ffff0000ffff,  // when var4 is at 0
  0x00000000ffffffff}; // when var5 is at 0
 */

/* return i if n == 2^i and i <= 6, 0 otherwise */
inline uint8_t power_two( const uint32_t n )  //returns the value of which to the power tow gives n ie the number of variables in function of the numbre of possible combinaisons
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
    default: return 0u;
  }
}

class Truth_Table
{
public:
  Truth_Table( uint8_t num_var )
   : num_var( num_var ), bits(  )
  {
	  //std::cout << "We are in the constructor Truth_Table( uint8_t num_var )" << std::endl;
      //bits.emplace_back(uint64_t(0));
      if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				bits.emplace_back(0x0000000000000000);
			}
		}
		else {
			bits.emplace_back(0x0000000000000000);
		}
      //std::cout << "We are in the constructor Truth_Table( uint8_t num_var ) for the second test" << std::endl;

    //assert( num_var <= 6u );  //have to be deleted it allow more than 6 variable truthtable
  }

  Truth_Table( uint8_t num_var, std::vector<uint64_t> bits )
   : num_var( num_var ), bits( AND_VECTOR(bits, length_mask(num_var)) )
  {
    //assert( num_var <= 6u );
  }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits(  )   //when giving to the truth table a list of ascii, we are able to determine th number of variables
  {
	if (num_var > 5) {
			for (uint8_t i = 0; i < (1 << (num_var-6)); i++){
				bits.emplace_back(0x0000000000000000);
			}
		}
		else {
			bits.emplace_back(0x0000000000000000);
		}
    if ( num_var == 0u ) // if length is not in power_tow
    {
      return;
    }

    for ( auto i = 0u; i < str.size(); ++i ) // we write the truth table 
    {
      if ( str[i] == '1' )
      {
        set_bit_VECTOR( str.size() - 1 - i );  // by setting at one
      }
      else
      {
        assert( str[i] == '0' );  // or checking that the value is indeed 0
      }
    }
  }

  bool get_bit_VECTOR( uint32_t const position ) const
  {
    assert( position < ( 1 << num_var ) );
    uint32_t pos_vector = position / 64; // the .at() where it is
    uint32_t pos_64bit = position % 64;  // the index in the .at() where it is
    return (  (bits.at(pos_vector) >> pos_64bit ) & 0x1 );
  }

  void set_bit_VECTOR( uint8_t const position )  // used to set a bit at 1 in an uint64_t
  {
    //assert( position < ( 1 << num_var ) ); will have problems for more than 64 bit long truth table
    
    std::vector<uint64_t> mask;
    
	if (num_var<7){
		mask.emplace_back(uint64_t( 1 ) << position);
		}

	else { // we have more than 6 variables we return a table of 2^(num_var-6)
		for (int i = 0; i < (0x00000001 << (6-num_var)); i++){
			if ( position < 64*(i+1) and position > 64*i ) {
				mask.emplace_back(uint64_t( 1 ) << (position-64*i)); 
				}
			else {
				mask.emplace_back(uint64_t( 0 )); 
				}
			}
		}

    bits = OR_VECTOR( bits, mask ) ;
    bits = AND_VECTOR( bits, length_mask(num_var) ) ;
  }

  uint8_t n_var() const  // to access num_var, which is public ... used in some tests
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
  for ( int32_t i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
  {
    os << ( tt.get_bit_VECTOR( i ) ? '1' : '0' );
  }
  return os;
}


/* list of operations which should be adaptedto fullfill more than & variables operations */



/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
	std::vector<uint64_t> result;
	for (unsigned i=0; i<tt.bits.size(); i++){
		result.emplace_back(~tt.bits.at(i));
		}
    return Truth_Table( tt.num_var, result ) ;   //return the bitwise not of the truthtable
}
/*
inline std::vector<uint64_t> operator~( std::vector<uint64_t> opa ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opa.size(); i++){
		result.emplace_back(~opa.at(i));
		}
	return result;
	}*/

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	std::vector<uint64_t> result;
	for (unsigned i=0; i<tt1.bits.size(); i++){
		result.emplace_back(tt2.bits.at(i) | tt1.bits.at(i));
		}
	return Truth_Table( tt1.num_var, result );
}
/*
inline std::vector<uint64_t> operator|( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) | opb.at(i));
		}
	return result;
	}*/

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	std::vector<uint64_t> result;
	for (unsigned i=0; i<tt2.bits.size(); i++){
		result.emplace_back(tt1.bits.at(i) & tt2.bits.at(i));
		}
	return Truth_Table( tt1.num_var, result );
}
/*
inline std::vector<uint64_t> operator&( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) & opb.at(i));
		}
	return result;
	}*/
	

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	assert( tt1.num_var == tt2.num_var );
	std::vector<uint64_t> result;
	for (unsigned i=0; i<tt1.bits.size(); i++){
		result.emplace_back(tt1.bits.at(i) ^ tt2.bits.at(i));
		}
	return Truth_Table( tt1.num_var, result );
}
/*
inline std::vector<uint64_t> operator^( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opb.size(); i++){
		result.emplace_back(opa.at(i) ^ opb.at(i));
		}
	return result;
	}*/

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	if ( tt1.num_var != tt2.num_var )
	{
		return false;
	}
	for (unsigned i=0; i<tt1.bits.size(); i++){
		if (tt1.bits.at(i) != tt2.bits.at(i))
		return false;
		}
	return true;
  
  ;
}
/*
inline bool operator== ( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	for (unsigned i=0; i<opb.size(); i++){
		if (opa.at(i) != opb.at(i))
		return false;
		}
	return true;
	}*/

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
	for (unsigned i=0; i<tt1.bits.size(); i++){
		if (tt1.bits.at(i) != tt2.bits.at(i))
		return true;
		}
	return false;
}
/*
inline bool operator!= ( std::vector<uint64_t> opa, std::vector<uint64_t> opb ){
	for (unsigned i=0; i<opb.size(); i++){
		if (opa.at(i) != opb.at(i))
		return true;
		}
	return false;
	}
	
inline std::vector<uint64_t> operator>> ( std::vector<uint64_t> opa, uint8_t shift ){
	std::vector<uint64_t> result;
	for (unsigned i=0; i<opa.size(); i++){
		if (i + 1 < opb.size() ){
			result.emplace_back( (opa.at(i) >> shift) | opa.at(i)<< (64-shift));
		}
		else {
			result.emplace_back (opa.at(i) >> shift);
		}
	return result;
	}*/

inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  return Truth_Table( num_var, OR_VECTOR( ( AND_VECTOR (bits, var_mask_pos(var, num_var) ) ) , R_SHIFT_VECTOR( AND_VECTOR( bits, var_mask_pos(var, num_var) ), ( 1 << var ) ) ) );
}

inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
{
  assert( var < num_var );
  return Truth_Table( num_var, OR_VECTOR( ( AND_VECTOR (bits, var_mask_neg(var, num_var) ) ) , L_SHIFT_VECTOR( AND_VECTOR( bits, var_mask_neg(var, num_var) ), ( 1 << var ) ) ) );
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
  assert ( var < num_var );
  //std::cout << "In create_tt_nth_var, having var = " << int(var) << " num_var = " << int(num_var) << "  returing " << (polarity ? var_mask_pos(var, num_var) : var_mask_neg(var, num_var)).at(0)%4 << std::endl;
  return Truth_Table( num_var, polarity ? var_mask_pos(var, num_var) : var_mask_neg(var, num_var) );
}
