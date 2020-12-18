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
        switch( n )           // extent for truth tables up to 32 bits
        {
        case 2u: return 1u;
        case 4u: return 2u;
        case 8u: return 3u;
        case 16u: return 4u;
        case 32u: return 5u;
        case 64u: return 6u;
        case 128u: return 7u; // nex cases for n>6
        case 256u: return 8u;
        case 512u: return 9u;
        case 1024u: return 10u;
        case 2048u: return 11u;
        case 4096u: return 12u;
        case 8192u: return 13u;
        case 16384u: return 14u;
        default: return 0u;
        
    }
}


class Truth_Table
{
public:
    
    Truth_Table( uint8_t num_var, std::vector<bool> bits )  //necessary to treat the tt as bool vector
    : num_var( num_var ),
    size_tt(bits.size()),
    bits(bits)
    {
    }
    
    Truth_Table( uint8_t num_var )
    : num_var( num_var ),
    size_tt(1<<num_var), //initialization of the size of the tt
    bits(size_tt, false) //initialization of the bool vector bits that store the tt
    {
    }
    
    Truth_Table( uint8_t num_var, uint64_t bits_unit64 )
    : num_var( num_var ),
    size_tt(1<<num_var)
    {
        if (bits_unit64==1){this -> bits.push_back(true);} //if read a 1 put true
        else{this -> bits.push_back(false);} //else put false

    }
    
    Truth_Table( const std::string str )
    : size_tt(str.size()), //put sire_tt = size of the string tt
    num_var( power_two( str.size() ) ) //use power tow to calculate the number of var and put it in num_var
    {
        if ( num_var == 0u ){return;} //if num var is 0 return nothin
        for ( int i = 0u; i < str.size(); ++i )
        {
           bits.push_back(str[i] == '1'); // create the bool tt from the string// if read 1 in the string put a true in bool tt else put false
        }
    }


    bool get_bit( uint8_t const position ) const
    {
        return bits[size_tt - position - 1]; //return ture or false according to the position in the tt bool
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
    uint64_t const size_tt; //size of the tt
    std::vector<bool> bits; //the new tt
    
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
    std::vector<bool> inv; //intermediary bool vector
    for (int i=0 ; i < tt.bits.size() ; i++) //read the tt initial
    {
        inv.push_back(!tt.bits[i]); //put the contratiy as a bool (NOT operator)
    }
    return Truth_Table( tt.num_var, inv ); //return the tt
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    std::vector<bool> ttor;
    for (int i = 0; i < tt1.size_tt; i++) {
        ttor.push_back(tt1.bits[i] || tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, ttor );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    std::vector<bool> ttand;
    for (int i = 0; i < tt1.size_tt; i++) {
        ttand.push_back(tt1.bits[i] && tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, ttand );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    std::vector<bool> ttxor;
    for (int i = 0; i < tt1.size_tt; i++) {
        ttxor.push_back(tt1.bits[i] ^ tt2.bits[i]);
    }
    return Truth_Table( tt1.num_var, ttxor );
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


inline Truth_Table Truth_Table::derivative( uint8_t const var ) const
{
    return positive_cofactor( var ) ^ negative_cofactor( var );
}

inline Truth_Table Truth_Table::consensus( uint8_t const var ) const
{
    return positive_cofactor( var ) & negative_cofactor( var );
}

inline Truth_Table Truth_Table::smoothing( uint8_t const var ) const
{
    return positive_cofactor( var ) | negative_cofactor( var );
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const neg_pos = true ) // create the mask on place
{
    auto recursive = 1 << var;//according to the varible we can deduce a recursive manner to create the mask
    std::vector<bool> mask_computed;
    for(auto i=0u; i < (1<<num_var);  i = i + 2*recursive )
    {
        for (auto k = 0u ; k < recursive; k++)
        {
            mask_computed.push_back(neg_pos);
        }
        for (auto k = 0u; k < recursive ; k++)
        {
            mask_computed.push_back(!neg_pos);
        }
    }
    return Truth_Table( num_var, mask_computed ); //return the tt that corresponds to the generated mask
}


    


