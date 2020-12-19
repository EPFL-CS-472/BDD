#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <set>

std::map<std::vector<bool>, uint64_t> build_map(std::map<std::vector<bool>, uint64_t>, std::vector<bool>, uint32_t);

std::map<std::vector<bool>, uint64_t> build_map(uint32_t num_var);

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
        0xffffffff00000000,
        0xffffffffffffffff
};

/* masks used to get the bits where a certain variable is 0 */
static const uint64_t var_mask_neg[] = {
        0x5555555555555555,
        0x3333333333333333,
        0x0f0f0f0f0f0f0f0f,
        0x00ff00ff00ff00ff,
        0x0000ffff0000ffff,
        0x00000000ffffffff,
        0xffffffffffffffff
};

/* return i if n == 2^i and i <= 6, 0 otherwise */
inline uint8_t power_two( const uint32_t n ) {
    uint8_t i = 1;
    uint32_t j;
    for (j = 2; j < n && j > 0; j = j << 1) {
        i++;
    }
    if (j == n) {
        return i;
    }
    return 0;
}

class Truth_Table {
public:

    explicit Truth_Table(uint32_t num_var): num_var(num_var) {
        this->bits = build_map(num_var);
    }

    Truth_Table(uint8_t num_var, std::map<std::vector<bool>, uint64_t> bits): num_var(num_var), bits(std::move(bits)) {
    }

    explicit Truth_Table(const std::string& str): num_var(power_two(str.size())) {
        if (num_var == 0u) {
            return;
        }
        this->bits = build_map(num_var);

        auto index_size = num_var > 6 ? num_var - 6 : 0;
        std::vector<bool> index(index_size, true);

        if (num_var <= 6) {
            for (auto i = 0u; i < str.size(); ++i) {
                if (str[i] == '1') {
                    bits[index] = set_bit(bits[index], str.size() - 1 - i);
                } else {
                    assert(str[i] == '0');
                }
            }
            return;
        }

        for (auto i = 0u; i < str.size(); ++i) {
            if (str[i] == '1') {
                auto position = (str.size() - 1 - i) % 64;
                bits[index] = set_bit(bits[index], position);
            } else {
                assert(str[i] == '0');
            }
            if (i % 64 != 0) {
                continue;
            }
            uint32_t pos;
            for (pos = 0; pos < index_size && !(index[pos]); pos++) {
                index[pos] = !index[pos];
            }
            index[pos] = !index[pos];
        }
    }

    static bool get_bit(uint64_t block, uint8_t const position) {
        assert(position < (1 << 6));
        return ((block >> position) & 0x1);
    }

    void write_block(std::ostream &ostream, uint64_t block) const {
        for (int8_t i = (1 << (num_var % 7)) - 1; i >= 0; --i) {
            ostream << (get_bit(block, i) ? '1' : '0');
        }
    }

    uint64_t set_bit(uint64_t block, uint8_t const position) const {
        assert(position < (1 << 6));
        block |= (uint64_t(1) << position);
        if (num_var >= 6)
            return block & length_mask[6];
        return (block & length_mask[num_var]);
    }

    uint8_t n_var() const {
        return num_var;
    }

public:
    uint32_t const num_var; /* number of variables involved in the function */
    std::map<std::vector<bool>, uint64_t> bits; /* the truth table, indexed by variables with index greater than 6*/
};

inline std::ostream& write_blocks(std::ostream& os, Truth_Table const& tt, const std::vector<bool>& index) {
    if (index.size() <= tt.num_var - 6) {
        tt.write_block(os, tt.bits.find(index)->second);
        return os;
    }
    auto then_index = index;
    auto else_index = index;
    then_index.push_back(true);
    else_index.push_back(false);
    write_blocks(os, tt, then_index);
    write_blocks(os, tt, else_index);
    return os;
}

/* overload std::ostream operator for convenient printing */
inline std::ostream& operator<<(std::ostream& os, Truth_Table const& tt ) {
    if (tt.num_var <= 6) {
        tt.write_block(os, tt.bits.find(std::vector<bool>())->second);
        return os;
    }
    write_blocks(os, tt, std::vector<bool>());
    return os;
}

/* bit-wise NOT operation */
inline Truth_Table operator~( Truth_Table const& tt ) {
    auto mask = tt.num_var >= 6 ? length_mask[6] : length_mask[tt.num_var];
    std::map<std::vector<bool>, uint64_t> bits;
    for (const auto& pair: tt.bits) {
        bits[pair.first] = (~pair.second) & mask;
    }
    return Truth_Table(tt.num_var, bits);
}

/* bit-wise OR operation */
inline Truth_Table operator|( Truth_Table const& tt1, Truth_Table const& tt2 ) {
    assert(tt1.num_var == tt2.num_var);
    auto mask = tt1.num_var >= 6 ? length_mask[6] : length_mask[tt1.num_var];
    std::map<std::vector<bool>, uint64_t> bits;
    for (const auto &pair: tt1.bits) {
        auto value = tt2.bits.find(pair.first)->second;
        bits[pair.first] = (pair.second | value) & mask;
    }
    return Truth_Table(tt1.num_var, bits);
}

/* bit-wise AND operation */
inline Truth_Table operator&( Truth_Table const& tt1, Truth_Table const& tt2 ) {
    assert(tt1.num_var == tt2.num_var);
    auto mask = tt1.num_var >= 6 ? length_mask[6] : length_mask[tt1.num_var];
    std::map<std::vector<bool>, uint64_t> bits;
    for (const auto &pair: tt1.bits) {
        auto value = tt2.bits.find(pair.first)->second;
        bits[pair.first] = (pair.second & value) & mask;
    }
    return Truth_Table(tt1.num_var, bits);
}

/* bit-wise XOR operation */
inline Truth_Table operator^( Truth_Table const& tt1, Truth_Table const& tt2 ) {
    assert(tt1.num_var == tt2.num_var);
    auto mask = tt1.num_var >= 6 ? length_mask[6] : length_mask[tt1.num_var];
    std::map<std::vector<bool>, uint64_t> bits;
    for (const auto &pair: tt1.bits) {
        auto value = tt2.bits.find(pair.first)->second;
        bits[pair.first] = (pair.second ^ value) & mask;
    }
    return Truth_Table(tt1.num_var, bits);
}

/* check if two truth_tables are the same */
inline bool operator==( Truth_Table const& tt1, Truth_Table const& tt2 ) {
    if (tt1.num_var != tt2.num_var) {
        return false;
    }
    return tt1.bits == tt2.bits;
}

inline bool operator!=( Truth_Table const& tt1, Truth_Table const& tt2 ) {
    return !(tt1 == tt2);
}

std::map<std::vector<bool>, uint64_t> build_map(
        std::map<std::vector<bool>, uint64_t> map,
        std::vector<bool> index,
        uint32_t num_vars) {
    if (num_vars <= 6) {
        map[std::vector<bool>()] = 0u;
        return map;
    }
    if (index.size() + 6 == num_vars) {
        auto index_copy = index;
        index.push_back(true);
        index_copy.push_back(false);
        map[index] = 0u;
        map[index_copy] = 0u;
        return map;
    }
    auto index_copy = index;
    index.push_back(true);
    index_copy.push_back(false);
    return build_map(build_map(map, index, num_vars), index_copy, num_vars);
}

std::map<std::vector<bool>, uint64_t> build_map(uint32_t num_var) {
    return build_map(std::map<std::vector<bool>, uint64_t>(), std::vector<bool>(), num_var);
}

/* Returns the truth table of f(x_0, ..., x_num_var) = x_var (or its mask). */
inline Truth_Table create_tt_nth_var(uint32_t const num_var, uint32_t const var, bool const polarity = true ) {
    assert (var < num_var);
    auto bits = build_map(num_var);
    auto mask_position = var > 6 ? 6 : var;
    auto mask = polarity ? var_mask_pos[mask_position] : var_mask_neg[var];
    for (const auto& pair: bits) {
        if (var < 6) {
            bits[pair.first] = mask;
        } else if (!pair.first.empty() && pair.first[var - 6]) {
            bits[pair.first] = mask;
        }
    }
    return Truth_Table(num_var, bits);
}