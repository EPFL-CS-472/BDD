#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <vector>

/* masks used to get the bits where a certain variable is 1 */
static const uint64_t var_mask_pos[] = {
  0xaaaaaaaaaaaaaaaa,
  0xcccccccccccccccc,
  0xf0f0f0f0f0f0f0f0,
  0xff00ff00ff00ff00,
  0xffff0000ffff0000,
  0xffffffff00000000 };

/* masks used to get the bits where a certain variable is 0 */
static const uint64_t var_mask_neg[] = {
  0x5555555555555555,
  0x3333333333333333,
  0x0f0f0f0f0f0f0f0f,
  0x00ff00ff00ff00ff,
  0x0000ffff0000ffff,
  0x00000000ffffffff };

/* return i if n == 2^i, 0 otherwise */
uint8_t power_two(const uint32_t n) {
  uint32_t curr = n;
  uint8_t i = 0u;
  while ((curr & 1) == 0) {
    curr = curr >> 1; ++i;
  }
  assert(curr == 1);
  return i;
}

std::vector<uint64_t> mask_pos(const uint8_t num_var, const uint8_t var) {
  std::vector<uint64_t> bits((uint64_t(1) << (num_var - 6u)));
  for (auto i = 0u; i < bits.size(); ++i) {
    if (var < 6u) {
      bits[i] = var_mask_pos[var];
    } else {
      if (((i >> (var - 6u)) & 1) == 0) {
        bits[i] = uint64_t(0);
      } else {
        bits[i] = uint64_t(-1);
      }
    }
  }
  return bits;
}

std::vector<uint64_t> mask_neg(const uint8_t num_var, const uint8_t var) {
  std::vector<uint64_t> bits((uint64_t(1) << (num_var - 6u)));
  for (auto i = 0u; i < bits.size(); ++i) {
    if (var < 6u) {
      bits[i] = var_mask_neg[var];
    } else {
      if (((i >> (var - 6u)) & 1) == 0) {
        bits[i] = uint64_t(-1);
      } else {
        bits[i] = uint64_t(0);
      }
    }
  }
  return bits;
}

class Truth_Table {
public:
  Truth_Table(uint8_t num_var)
    : num_var(num_var), bits(0u) {
    assert(num_var <= 64u);
  }

  Truth_Table(uint8_t num_var, std::vector<uint64_t> bits)
    : num_var(num_var), bits(bits) {
    assert(num_var <= 64u);
  }

  Truth_Table(const std::string str)
    : num_var(power_two(str.size())), bits(str.size() / 64u) {
    assert(num_var <= 64u);

    for (auto i = 0u; i < str.size(); ++i) {
      if (str[i] == '1') {
        set_bit(str.size() - 1 - i);
      } else {
        assert(str[i] == '0');
      }
    }
  }

  bool get_bit(uint64_t const position) const {
    assert(num_var == 64u | position < (uint64_t(1) << num_var));
    return ((bits[position / 64u] >> (position % 64u)) & 1);
  }

  void set_bit(uint64_t const position) {
    assert(num_var == 64u | position < (uint64_t(1) << num_var));
    bits[position / 64u] |= (uint64_t(1) << (position % 64u));
  }

  uint8_t n_var() const {
    return num_var;
  }

  Truth_Table positive_cofactor(uint8_t const var) const;
  Truth_Table negative_cofactor(uint8_t const var) const;
  Truth_Table derivative(uint8_t const var) const;
  Truth_Table consensus(uint8_t const var) const;
  Truth_Table smoothing(uint8_t const var) const;

public:
  uint8_t const num_var; /* number of variables involved in the function */
  std::vector<uint64_t> bits; /* the truth table */
};

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<(std::ostream& os, Truth_Table const& tt) {
  for (int64_t i = (uint64_t(1) << tt.num_var) - 1; i >= 0; --i) {
    os << (tt.get_bit(i) ? '1' : '0');
  }
  return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~(Truth_Table const& tt) {
  std::vector<uint64_t> bits(tt.bits.size());
  for (auto i = 0u; i < bits.size(); ++i) {
    bits[i] = ~tt.bits[i];
  }
  return Truth_Table(tt.num_var, bits);
}

/* bit-wise OR operation */
inline Truth_Table operator|(Truth_Table const& tt1, Truth_Table const& tt2) {
  assert(tt1.num_var == tt2.num_var);
  std::vector<uint64_t> bits(tt1.bits.size());
  for (auto i = 0u; i < bits.size(); ++i) {
    bits[i] = tt1.bits[i] | tt2.bits[i];
  }
  return Truth_Table(tt1.num_var, bits);
}

/* bit-wise AND operation */
inline Truth_Table operator&(Truth_Table const& tt1, Truth_Table const& tt2) {
  assert(tt1.num_var == tt2.num_var);
  std::vector<uint64_t> bits(tt1.bits.size());
  for (auto i = 0u; i < bits.size(); ++i) {
    bits[i] = tt1.bits[i] & tt2.bits[i];
  }
  return Truth_Table(tt1.num_var, bits);
}

/* bit-wise XOR operation */
inline Truth_Table operator^(Truth_Table const& tt1, Truth_Table const& tt2) {
  assert(tt1.num_var == tt2.num_var);
  std::vector<uint64_t> bits(tt1.bits.size());
  for (auto i = 0u; i < bits.size(); ++i) {
    bits[i] = tt1.bits[i] ^ tt2.bits[i];
  }
  return Truth_Table(tt1.num_var, bits);
}

/* check if two truth_tables are the same */
inline bool operator==(Truth_Table const& tt1, Truth_Table const& tt2) {
  if (tt1.num_var != tt2.num_var) {
    return false;
  }
  return tt1.bits == tt2.bits;
}

inline bool operator!=(Truth_Table const& tt1, Truth_Table const& tt2) {
  return !(tt1 == tt2);
}

inline Truth_Table Truth_Table::positive_cofactor(uint8_t const var) const {
  assert(var < num_var);
  const std::vector<uint64_t> mask = mask_pos(num_var, var);
  std::vector<uint64_t> masked_bits(mask.size());
  for (auto i = 0u; i < masked_bits.size(); ++i) {
    masked_bits[i] = bits[i] & mask[i];
  }
  std::vector<uint64_t> new_bits(bits.size());
  for (auto i = 0u; i < new_bits.size(); ++i) {
    if (var < 6u) {
      new_bits[i] = masked_bits[i]| (masked_bits[i] >> (1 << var));
    } else {
      new_bits[i] = masked_bits[i];
      if (i + (1 << (var - 6)) < new_bits.size())
        new_bits[i] |= masked_bits[i + (1 << (var - 6))];
    }
  }
  return Truth_Table(num_var, new_bits);
}

inline Truth_Table Truth_Table::negative_cofactor(uint8_t const var) const {
  assert(var < num_var);
  const std::vector<uint64_t> mask = mask_neg(num_var, var);
  std::vector<uint64_t> masked_bits(mask.size());
  for (auto i = 0u; i < masked_bits.size(); ++i) {
    masked_bits[i] = bits[i] & mask[i];
  }
  std::vector<uint64_t> new_bits(bits.size());
  for (auto i = 0u; i < new_bits.size(); ++i) {
    if (var < 6u) {
      new_bits[i] = masked_bits[i] | (masked_bits[i] << (1 << var));
    } else {
      new_bits[i] = masked_bits[i];
      if (i - (1 << (var - 6)) >= 0)
        new_bits[i] |= masked_bits[i - (1 << (var - 6))];
    }
  }
  return Truth_Table(num_var, new_bits);
}

inline Truth_Table Truth_Table::derivative(uint8_t const var) const {
  assert(var < num_var);
  return positive_cofactor(var) ^ negative_cofactor(var);
}

inline Truth_Table Truth_Table::consensus(uint8_t const var) const {
  assert(var < num_var);
  return positive_cofactor(var) & negative_cofactor(var);
}

inline Truth_Table Truth_Table::smoothing(uint8_t const var) const {
  assert(var < num_var);
  return positive_cofactor(var) | negative_cofactor(var);
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its complement). */
inline Truth_Table create_tt_nth_var(uint8_t const num_var, uint8_t const var, bool const polarity = true) {
  assert(num_var <= 64u && var < num_var);

  const std::vector<uint64_t> bits(polarity ? mask_pos(num_var, var) : mask_neg(num_var, var));
  return Truth_Table(num_var, bits);
}