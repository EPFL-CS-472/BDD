#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

/* return i if n == 2^i and i <= 31, 0 otherwise */
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
    case 128u : return 7u;
    case 256u : return 8u;
    case 512u : return 9u;
    case 1024u : return 10u;
    case 2048u : return 11u;
    case 4096u : return 12u;
    case 8192u : return 13u;
    case 16384u : return 14u;
    case 32768u : return 15u;
    case 65536u : return 16u;
    case 131072u : return 17u;
    case 262144u : return 18u;
    case 524288u : return 19u;
    case 1048576u : return 20u;
    case 2097152u : return 21u;
    case 4194304u : return 22u;
    case 8388608u : return 23u;
    case 16777216u : return 24u;
    case 33554432u : return 25u;
    case 67108864u : return 26u;
    case 134217728u : return 27u;
    case 268435456u : return 28u;
    case 536870912u : return 29u;
    case 1073741824u : return 30u;
    case 2147483648u : return 31u;    
    default: return 0u;
  }
}

class Truth_Table
{
public:
    Truth_Table( uint8_t num_var )
    : num_var( num_var ), vectTT(size_tt, false), size_tt(1<<num_var)
    {
    }
    
    Truth_Table( uint8_t num_var, uint64_t vectTT )
    : num_var( num_var ), size_tt(1<<num_var)
    {
        for (auto i = 1; i<=size_tt; ++i) {
            bool bit_convert = vectTT>>(size_tt-i) & 1; //convertion of a uint64_t bit to a bool
            this->vectTT.push_back(bit_convert);
        }
    }
    

    Truth_Table( const std::string str )
    : num_var( power_two( str.size() ) ), size_tt(str.size())
    {
        if ( num_var == 0u ){
            return;
        }
        //bits from the str go to the bool vector        
        for ( int i = 0; i < str.size(); ++i ) {
            vectTT.push_back(str[i] == '1');
        }
    }

    Truth_Table( uint8_t num_var, std::vector<bool> vectTT )
    : num_var( num_var ), vectTT(vectTT), size_tt(vectTT.size())
    {
    }
    
    uint8_t n_var() const
    {
        return num_var;
    }
    
public:
    uint8_t const num_var; /* number of variables involved in the function */
    uint64_t const size_tt; // the size of the truth table
    std::vector<bool> vectTT; // the new truth table which is now a vector of boolean instead of a unit64_t in order to avoid any size limit
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<( std::ostream& os, Truth_Table const& tt )
{
    for ( int8_t i = ( 1 << tt.num_var ) - 1; i >= 0; --i )
    {
        os << ( tt.vectTT[tt.size_tt - i - 1] ? '1' : '0' );        
    }
    return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt )
{
    std::vector<bool> not_tt;
    for (int i = 0; i < tt.vectTT.size(); i++) 
    {
        not_tt.push_back(not tt.vectTT[i]);
    }
    return Truth_Table( tt.num_var, not_tt );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> or_tt;
    for (auto i = 0u; i<tt1.size_tt; i++) 
    {
        or_tt.push_back(tt1.vectTT[i] or tt2.vectTT[i]);
    }
    return Truth_Table( tt1.num_var, or_tt );
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> and_tt;
    for (auto i = 0u; i<tt1.size_tt; i++)
    {
        and_tt.push_back(tt1.vectTT[i] and tt2.vectTT[i]);
    }
    return Truth_Table( tt1.num_var, and_tt );
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    assert( tt1.num_var == tt2.num_var );
    std::vector<bool> xor_tt;
    for (auto i = 0u; i<tt1.size_tt; i++)
    {
        xor_tt.push_back(tt1.vectTT[i] xor tt2.vectTT[i]);
    }
    return Truth_Table( tt1.num_var, xor_tt );
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    return tt1.num_var == tt2.num_var and tt1.vectTT == tt2.vectTT;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 )
{
    return not( tt1 == tt2 );
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{    
    // the masks are created for each call since we can not anymore use the former mask made only for up to 6 variables
    std::vector<bool> new_mask;
    int blocs = 1<<var;
    int mask_size = 1<<num_var;
    int i = 0;
    while (i<mask_size) {
        for (int j = 0; j < blocs; j++) {
            new_mask.push_back(polarity);
        }
        for (int j = 0; j < blocs; j++) {
            new_mask.push_back(not polarity);
        }
        i = i + 2*blocs;
    }
    
    return Truth_Table( num_var, new_mask );
}
