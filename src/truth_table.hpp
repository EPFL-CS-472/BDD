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


/* return i if n == 2^i and i <= 6, 0 otherwise */
inline uint8_t power_two( const uint32_t n )
{
        switch( n )   		// extent for truth tables up to 32 bits
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


class Truth_Table
{
public:
    Truth_Table( uint8_t num_var )
    : num_var( num_var ), bits_size(1<<num_var), bits(bits_size, false) 
    {
        
    }
    
    Truth_Table( uint8_t num_var, uint64_t bits )
    : num_var( num_var ), bits_size(1<<num_var) 
    {
        for (auto i = 1; i <= bits_size; i++) {

            if (bits==1)
	    {
		this -> bits.push_back(true) ;
	    }
	    else
	    {
		this -> bits.push_back(false) ;
	    }

        }
    }
    
    
    
    Truth_Table( const std::string str )
    : bits_size(str.size()), num_var( power_two( str.size() ) )
    {
        if ( num_var == 0u )
        {
            return;
        }
        
        for ( auto i = 0u; i < str.size(); ++i )
        {
           bits.push_back(str[i] == '1');
        }
    }


    Truth_Table( uint8_t num_var, std::vector<bool> bits )    // new instantiation since bits is type std::vector<bool>
    : num_var( num_var ), bits_size(bits.size()), bits(bits)
    {
    }

    
    bool get_bit( uint8_t const position ) const
    {
        return bits[bits_size - position - 1];
    }
    
//    void set_bit( uint8_t const position )
//    {
//        assert( position < ( 1 << num_var ) );
//        bits |= ( uint64_t( 1 ) << position );
//        bits &= length_mask[num_var];
//    }
    
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
    
    uint64_t const bits_size; 

    std::vector<bool> bits; /* the truth table */
    
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
    std::vector<bool> complemented;
    for (auto i=0 ; i < tt.bits.size() ; i++)
    {
        complemented.push_back(!tt.bits[i]);   
    }
    return Truth_Table( tt.num_var, complemented );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> tt_OR;

    for (auto i = 0u; i < tt1.bits_size; i++) {
        tt_OR.push_back(tt1.bits[i] || tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, tt_OR );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> tt_AND;
    for (auto i = 0u; i < tt1.bits_size; i++) {
        tt_AND.push_back(tt1.bits[i] && tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, tt_AND );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> tt_XOR;
    for (auto i = 0u; i < tt1.bits_size; i++) {
        tt_XOR.push_back(tt1.bits[i] ^ tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, tt_XOR );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    if ( tt1.num_var != tt2.num_var )
    {
        return false;
    }
    return tt1.bits == tt2.bits;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    return !( tt1 == tt2 );
}


//inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
//{
//  assert( var < num_var );
//  return Truth_Table( num_var, ( bits & var_mask_pos[var] ) | ( ( bits & var_mask_pos[var] ) >> ( 1 << var ) ) );
//}

//inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
//{
//  assert( var < num_var );
//  return Truth_Table( num_var, ( bits & var_mask_neg[var] ) | ( ( bits & var_mask_neg[var] ) << ( 1 << var ) ) );
//}


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
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true ) // create mask_pos or mask_neg 
{
    assert ( var < num_var );
    
    std::vector<bool> mask;

    auto pattern = 1 << var;

    auto i = 0u;

    while (i < (1<<num_var)) {
        for (auto j = 0u ; j < pattern; j++) 
        {
            mask.push_back(polarity);
        }
        for (auto j = 0u; j < pattern ; j++) {
            mask.push_back(!polarity);
        }
        i += 2*pattern ;
    }
    
    return Truth_Table( num_var, mask );
}
