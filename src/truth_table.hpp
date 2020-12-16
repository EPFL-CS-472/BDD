#pragma once

#include <iostream>
#include <cassert>
#include <string>

#include <vector>

/* return i if n == 2^i and i <= 6, 0 otherwise */
inline uint8_t power_two(uint32_t n)
{
  uint32_t res = 0;
  while(n != 0){
    auto last = n & 0x1;
    n >>= 1;
    if(last == 0 && n != 0) 
      res += 1;
    else if(last == 1 && n != 0){
      assert(false && "This should not work anymore!");
      return 0; // not exactly.
    }
  }
  return res;
}


class Truth_Table
{
public:
  Truth_Table( uint8_t num_var )
   : num_var( num_var ), bits(1 << num_var, false)
  {
    //assert( num_var <= 6u );
  }

  Truth_Table( uint8_t num_var, uint64_t bits )
   : num_var( num_var ), bits(1 << num_var, false)
  {
    //assert( num_var <= 6u );
    for (auto i = 0u; i < this->bits.size(); ++i) {
      this->bits[i] = ((bits & 0x1) != 0);
      bits >>= 1;
    }
  }

  Truth_Table(uint8_t num_var, std::vector<bool> &&b)
    : num_var(num_var), bits(b)
  {

  }

  Truth_Table( const std::string str )
   : num_var( power_two( str.size() ) ), bits(1 << num_var, false)
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
    return bits[position];
  }

  void set_bit( uint8_t const position )
  {
    assert( position < ( 1 << num_var ) );
    bits[position] = true;
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
  std::vector<bool> cpy = tt.bits;
  cpy.flip();
  return Truth_Table( tt.num_var, std::move(cpy) );
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );

  std::vector<bool> res = std::vector<bool>(1 << tt1.num_var, false);
  for(auto i = 0u; i < tt1.bits.size(); ++i){
    res[i] = (tt1.bits[i] || tt2.bits[i]);
  }

  return Truth_Table( tt1.num_var, std::move(res));
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );

  std::vector<bool> res = std::vector<bool>(1 << tt1.num_var, false);
  for(auto i = 0u; i < tt1.bits.size(); ++i){
    res[i] = (tt1.bits[i] && tt2.bits[i]);
  }

  return Truth_Table( tt1.num_var, std::move(res));
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 )
{
  assert( tt1.num_var == tt2.num_var );

  std::vector<bool> res = std::vector<bool>(1 << tt1.num_var, false);
  for(auto i = 0u; i < tt1.bits.size(); ++i){
    res[i] = (tt1.bits[i] != tt2.bits[i]);
  }

  return Truth_Table( tt1.num_var, std::move(res));
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

inline Truth_Table Truth_Table::positive_cofactor( uint8_t const var ) const
{
  assert( var < num_var );

  Truth_Table result{num_var};
  for(uint8_t i = 0; i < (1 << num_var); i += (2 << var)){
    // collect (1 << var) bits from zero and duplicate another.
    for(uint8_t j = (1 << var); j < (2 << var); ++j){
      if(this->get_bit(i + j)){
        result.set_bit(i + j);
        result.set_bit(i + j - (1 << var));
      }
    }
  }

  return result;
}

inline Truth_Table Truth_Table::negative_cofactor( uint8_t const var ) const
{
  assert( var < num_var );

  Truth_Table result{num_var};
  for(uint8_t i = 0; i < (1 << num_var); i += (2 << var)){
    // collect (1 << var) bits from zero and skip another.
    for(uint8_t j = 0; j < (1 << var); ++j){
      if(this->get_bit(i + j)){
        result.set_bit(i + j);
        result.set_bit(i + j + (1 << var));
      }
    }
  }

  return result;
}

inline Truth_Table Truth_Table::derivative( uint8_t const var ) const
{
  assert( var < num_var );

  auto pos = this->positive_cofactor(var);
  auto neg = this->negative_cofactor(var);

  return pos ^ neg;
}

inline Truth_Table Truth_Table::consensus( uint8_t const var ) const
{
  assert( var < num_var );
  
  auto pos = this->positive_cofactor(var);
  auto neg = this->negative_cofactor(var);

  return pos & neg;
}

inline Truth_Table Truth_Table::smoothing( uint8_t const var ) const
{
  assert( var < num_var );
  
  auto pos = this->positive_cofactor(var);
  auto neg = this->negative_cofactor(var);

  return pos | neg;
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var( uint8_t const num_var, uint8_t const var, bool const polarity = true )
{
  assert (var < num_var);
  auto res = Truth_Table(num_var);
  uint32_t pattern_size = (1u << var);
  for(auto i = 0u; i < (1u << num_var); i += 2 * pattern_size){
    for(auto j = 0u; j < pattern_size; ++j)
      res.bits[i + j] = !polarity;
    for(auto j = 0u; j < pattern_size; ++j)
      res.bits[i + pattern_size + j] = polarity;
  }
  return res;
}