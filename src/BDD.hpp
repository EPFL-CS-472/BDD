#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <utility>
#include <tuple>

/* These are just some hacks to hash std::pair and std::tuple.
 * You don't need to understand this part. */
namespace std {
    template<class T>
    inline void hash_combine(size_t &seed, T const &v) {
        seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<>
    struct hash<pair<uint32_t, uint32_t>> {
        using argument_type = pair<uint32_t, uint32_t>;
        using result_type = size_t;

        result_type operator()(argument_type const &in) const {
            result_type seed = 0;
            hash_combine(seed, in.first);
            hash_combine(seed, in.second);
            return seed;
        }
    };

    template<>
    struct hash<tuple<uint32_t, uint32_t>> {
        using argument_type = tuple<uint32_t, uint32_t>;
        using result_type = size_t;

        result_type operator()(argument_type const &in) const {
            result_type seed = 0;
            hash_combine(seed, std::get<0>(in));
            hash_combine(seed, std::get<1>(in));
            return seed;
        }
    };

    template<>
    struct hash<tuple<uint32_t, uint32_t, uint32_t>> {
        using argument_type = tuple<uint32_t, uint32_t, uint32_t>;
        using result_type = size_t;

        result_type operator()(argument_type const &in) const {
            result_type seed = 0;
            hash_combine(seed, std::get<0>(in));
            hash_combine(seed, std::get<1>(in));
            hash_combine(seed, std::get<2>(in));
            return seed;
        }
    };
}

class BDD {
public:
    using signal_t = uint32_t;
    /* A signal represents an edge pointing to a node.
     * The first 31 bits store the index of the node,
     * and the last bit records whether the edge is complemented or not.
     * See below `make_signal`, `get_index` and `branch_mask`. */

    using var_t = uint32_t;
    /* Declare `var_t` as an alias for an unsigned integer.
     * This datatype will be used for representing variables. */

private:
    using binary_cache_key = std::tuple<signal_t, signal_t>;
    using binary_cache_option = std::tuple<const std::unordered_map<binary_cache_key, signal_t> &, binary_cache_key, bool>;
    using binary_cache_options = std::vector<binary_cache_option>;
    using ternary_cache_key = std::tuple<signal_t, signal_t, signal_t>;
    using ternary_cache_option = std::tuple<const std::unordered_map<ternary_cache_key, signal_t> &, ternary_cache_key, bool>;
    using ternary_cache_options = std::vector<ternary_cache_option>;

    using index_t = uint32_t;
    /* Declare `index_t` as an alias for an unsigned integer.
     * This is just for easier understanding of the code.
     * This datatype will be used for node indices. */

    static const index_t COMPLEMENT_MASK = 0x1;

    struct Node {
        var_t v; /* corresponding variable */
        signal_t T; /* signal of THEN child (should not be complemented) */
        signal_t E; /* signal of ELSE child */
    };

    static inline signal_t make_signal(index_t index, bool complement = false) {
        return complement ? (index << 1) | COMPLEMENT_MASK : index << 1;
    }

    inline index_t get_index(signal_t signal) const {
        assert((signal >> 1) < nodes.size());
        return signal >> 1;
    }

    inline Node get_node(signal_t signal) const {
        return nodes[get_index(signal)];
    }

    static inline index_t branch_mask(signal_t signal) {
        return signal & COMPLEMENT_MASK;
    }

    static inline signal_t mask(signal_t signal, index_t bit_mask) {
        return signal ^ bit_mask;
    }

    template<class Key>
    inline static signal_t cached_computation(
            const std::vector<std::tuple<const std::unordered_map<Key, signal_t> &, Key, bool>> &pre_computed,
            std::unordered_map<Key, signal_t> &cache,
            const Key &original,
            const std::function<signal_t()> &func) {
        for (auto const &option: pre_computed) {
            const std::unordered_map<Key, signal_t> &cache_option = std::get<0>(option);
            const Key &key = std::get<1>(option);
            bool mask = std::get<2>(option);
            if (cache_option.find(key) != cache_option.end()) {
                return cache_option.find(key)->second ^ mask;
            }
        }
        auto result = func();
        cache[original] = result;
        return result;
    }

    static signal_t mask_branch(signal_t f, signal_t branch) {
        if (f == branch) {
            return branch;
        }
        return mask(branch, branch_mask(f));
    }

    signal_t AND_INTERNAL(signal_t f, signal_t g) {
        Node const &F = get_node(f);
        Node const &G = get_node(g);

        /* trivial cases */
        if (f == constant(false) || g == constant(false)) {
            return constant(false);
        }
        if (f == constant(true)) {
            return g;
        }
        if (g == constant(true)) {
            return f;
        }
        if (f == g) {
            return f;
        }
        if (f == NOT(g)) {
            return constant(false);
        }

        var_t x;
        signal_t f0, f1, g0, g1;
        if (F.v < G.v) /* F is on top of G */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = g1 = g;
        } else if (G.v < F.v) /* G is on top of F */
        {
            x = G.v;
            f0 = f1 = f;
            g0 = G.E;
            g1 = G.T;
        } else /* F and G are at the same level */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = G.E;
            g1 = G.T;
        }

        signal_t const r0 = AND(mask_branch(f, f0), mask_branch(g, g0));
        signal_t const r1 = AND(mask_branch(f, f1), mask_branch(g, g1));
        return unique(x, r1, r0);
    }

    signal_t OR_INTERNAL(signal_t f, signal_t g) {
        Node const &F = get_node(f);
        Node const &G = get_node(g);

        /* trivial cases */
        if (f == constant(true) || g == constant(true)) {
            return constant(true);
        }
        if (f == constant(false)) {
            return g;
        }
        if (g == constant(false)) {
            return f;
        }
        if (f == g) {
            return f;
        }
        if (f == NOT(g)) {
            return constant(true);
        }

        var_t x;
        signal_t f0, f1, g0, g1;
        if (F.v < G.v) /* F is on top of G */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = g1 = g;
        } else if (G.v < F.v) /* G is on top of F */
        {
            x = G.v;
            f0 = f1 = f;
            g0 = G.E;
            g1 = G.T;
        } else /* F and G are at the same level */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = G.E;
            g1 = G.T;
        }

        signal_t const r0 = OR(mask_branch(f, f0), mask_branch(g, g0));
        signal_t const r1 = OR(mask_branch(f, f1), mask_branch(g, g1));
        return unique(x, r1, r0);
    }

    signal_t XOR_INTERNAL(signal_t f, signal_t g) {
        Node const &F = get_node(f);
        Node const &G = get_node(g);

        /* trivial cases */
        if (f == g) {
            return constant(false);
        }
        if (f == constant(false)) {
            return g;
        }
        if (g == constant(false)) {
            return f;
        }
        if (f == constant(true)) {
            return NOT(g);
        }
        if (g == constant(true)) {
            return NOT(f);
        }
        if (f == NOT(g)) {
            return constant(true);
        }

        var_t x;
        signal_t f0, f1, g0, g1;
        if (F.v < G.v) /* F is on top of G */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = g1 = g;
        } else if (G.v < F.v) /* G is on top of F */
        {
            x = G.v;
            f0 = f1 = f;
            g0 = G.E;
            g1 = G.T;
        } else /* F and G are at the same level */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            g0 = G.E;
            g1 = G.T;
        }

        signal_t const r0 = XOR(mask_branch(f, f0), mask_branch(g, g0));
        signal_t const r1 = XOR(mask_branch(f, f1), mask_branch(g, g1));
        return unique(x, r1, r0);
    }

    signal_t ITE_INTERNAL(signal_t f, signal_t g, signal_t h) {
        Node const &F = get_node(f);
        Node const &G = get_node(g);
        Node const &H = get_node(h);

        /* trivial cases */
        if (f == constant(true)) {
            return g;
        }
        if (f == constant(false)) {
            return h;
        }
        if (g == h) {
            return g;
        }

        var_t x;
        signal_t f0, f1, g0, g1, h0, h1;
        if (F.v <= G.v && F.v <= H.v) /* F is not lower than both G and H */
        {
            x = F.v;
            f0 = F.E;
            f1 = F.T;
            if (G.v == F.v) {
                g0 = G.E;
                g1 = G.T;
            } else {
                g0 = g1 = g;
            }
            if (H.v == F.v) {
                h0 = H.E;
                h1 = H.T;
            } else {
                h0 = h1 = h;
            }
        } else /* F.v > min(G.v, H.v) */
        {
            f0 = f1 = f;
            if (G.v < H.v) {
                x = G.v;
                g0 = G.E;
                g1 = G.T;
                h0 = h1 = h;
            } else if (H.v < G.v) {
                x = H.v;
                g0 = g1 = g;
                h0 = H.E;
                h1 = H.T;
            } else /* G.v == H.v */
            {
                x = G.v;
                g0 = G.E;
                g1 = G.T;
                h0 = H.E;
                h1 = H.T;
            }
        }

        signal_t const r0 = ITE(mask_branch(f, f0), mask_branch(g, g0), mask_branch(h, h0));
        signal_t const r1 = ITE(mask_branch(f, f1), mask_branch(g, g1), mask_branch(h, h1));
        return unique(x, r1, r0);
    }

public:
    explicit BDD(uint32_t num_vars)
            : unique_table(num_vars), num_invoke_and(0u), num_invoke_or(0u),
              num_invoke_xor(0u), num_invoke_ite(0u) {
        nodes.emplace_back(Node({num_vars, 0, 0})); /* constant 1 */
        refs.emplace_back(0);
    }

    /**********************************************************/
    /***************** Basic Building Blocks ******************/
    /**********************************************************/

    uint32_t num_vars() const {
        return unique_table.size();
    }

    /* Get the constant signal. */
    signal_t constant(bool value) const {
        return value ? COMPLEMENT_MASK : 0;
    }

    /* Look up (if exist) or build (if not) the node with variable `var`,
     * THEN child `T`, and ELSE child `E`. */
    signal_t unique(var_t var, signal_t T, signal_t E) {
        assert(var < num_vars() && "Variables range from 0 to `num_vars - 1`.");
        assert(get_node(T).v > var && "With static variable order, children can only be below the node.");
        assert(get_node(E).v > var && "With static variable order, children can only be below the node.");

        /* Reduction rule: Identical children */
        if (T == E) {
            return T;
        }

        /* Look up in the unique table. */
        const auto original_position = unique_table[var].find({T, E});
        const auto inverted_positions = unique_table[var].find({NOT(T), NOT(E)});
        if (original_position != unique_table[var].end()) {
            /* The required node already exists. Return original_position. */
            return make_signal(original_position->second);
        } else if (inverted_positions != unique_table[var].end()) {
            return make_signal(inverted_positions->second, true);
        } else {
            /* Create a new node, insert original_position to the unique table and increment ref count for children. */
            ref(T);
            ref(E);
            index_t const new_index = nodes.size();
            nodes.emplace_back(Node({var, T, E}));
            refs.emplace_back(0);
            unique_table[var][{T, E}] = new_index;
            return make_signal(new_index);
        }
    }

    /* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
    signal_t literal(var_t var, bool complement = false) {
        return unique(var, constant(!complement), constant(complement));
    }

    /**********************************************************/
    /*********************** Ref & Deref **********************/
    /**********************************************************/
    signal_t ref(signal_t f) {
        if (f == constant(true)) {
            return f;
        }
        if (f == constant(false)) {
            return f;
        }

        auto index = get_index(f);
        assert(index < refs.size());
        refs[index] += 1;
        return f;
    }

    void deref(signal_t f) {
        if (f == constant(true)) {
            return;
        }
        if (f == constant(false)) {
            return;
        }

        auto index = get_index(f);
        assert(index < refs.size() && refs[index] > 0);
        refs[index] -= 1;
        if (refs[index] == 0) {
            auto node = get_node(f);
            deref(node.E);
            deref(node.T);
        }
    }

    /**********************************************************/
    /********************* BDD Operations *********************/
    /**********************************************************/

    /* Compute ~f */
    signal_t NOT(signal_t f) {
        return f ^ COMPLEMENT_MASK;
    }

    /* Compute f ^ g */
    signal_t XOR(signal_t f, signal_t g) {
        ++num_invoke_xor;

        binary_cache_options options{
                binary_cache_option(computed_table_XOR, binary_cache_key(f, g), false),
                binary_cache_option(computed_table_XOR, binary_cache_key(g, f), false),
                // functional equivalence
                binary_cache_option(computed_table_XOR, binary_cache_key(NOT(g), NOT(f)), false),
                binary_cache_option(computed_table_XOR, binary_cache_key(NOT(f), NOT(g)), false)
        };

        return cached_computation(
                options,
                computed_table_XOR,
                binary_cache_key(f, g),
                [this, f, g]() -> signal_t {
                    return XOR_INTERNAL(f, g);
                });
    }

    /* Compute f & g */
    signal_t AND(signal_t f, signal_t g) {
        ++num_invoke_and;

        binary_cache_options options{
                binary_cache_option(computed_table_AND, binary_cache_key(f, g), false),
                binary_cache_option(computed_table_AND, binary_cache_key(g, f), false),
                // De Morgan's
                binary_cache_option(computed_table_OR, binary_cache_key(NOT(f), NOT(g)), true),
                binary_cache_option(computed_table_OR, binary_cache_key(NOT(g), NOT(f)), true)
        };

        return cached_computation(
                options,
                computed_table_AND,
                binary_cache_key(f, g),
                [this, f, g]() -> signal_t {
                    return AND_INTERNAL(f, g);
                });
    }

    /* Compute f | g */
    signal_t OR(signal_t f, signal_t g) {
        ++num_invoke_or;

        binary_cache_options options{
                binary_cache_option(computed_table_OR, binary_cache_key(f, g), false),
                binary_cache_option(computed_table_OR, binary_cache_key(g, f), false),
                // De Morgan's
                binary_cache_option(computed_table_AND, binary_cache_key(NOT(f), NOT(g)), true),
                binary_cache_option(computed_table_AND, binary_cache_key(NOT(g), NOT(f)), true)
        };

        return cached_computation(
                options,
                computed_table_OR,
                binary_cache_key(f, g),
                [this, f, g]() -> signal_t {
                    return OR_INTERNAL(f, g);
                });
    }

    /* Compute ITE(f, g, h), i.e., f ? g : h */
    signal_t ITE(signal_t f, signal_t g, signal_t h) {
        ++num_invoke_ite;

        ternary_cache_options options{
                ternary_cache_option(computed_table_ITE, ternary_cache_key(f, g, h), false),
                ternary_cache_option(computed_table_ITE, ternary_cache_key(NOT(f), h, g), false),
                // functional equivalence
                ternary_cache_option(computed_table_ITE, ternary_cache_key(f, NOT(g), NOT(h)), true),
                ternary_cache_option(computed_table_ITE, ternary_cache_key(NOT(f), NOT(h), NOT(g)), true),
        };

        return cached_computation(
                options,
                computed_table_ITE,
                ternary_cache_key(f, g, h),
                [this, f, g, h]() -> signal_t {
                    return ITE_INTERNAL(f, g, h);
                });
    }

    /**********************************************************/
    /***************** Printing and Evaluating ****************/
    /**********************************************************/

    /* Print the BDD rooted at node `f`. */
    void print(signal_t f, std::ostream &os = std::cout) const {
        Node const &F = get_node(f);
        for (auto i = 0u; i < F.v; ++i) {
            os << "  ";
        }
        if (f <= 1) {
            os << "constant " << (f ? "0" : "1") << std::endl;
        } else {
            if (branch_mask(f)) {
                os << "!";
            } else {
                os << " ";
            }

            os << "node " << get_index(f) << ": var = " << F.v << ", T = " << F.T
               << ", E = " << F.E << std::endl;
            for (auto i = 0u; i < F.v; ++i) {
                os << "  ";
            }
            os << "> THEN branch" << std::endl;
            print(F.T, os);
            for (auto i = 0u; i < F.v; ++i) {
                os << "  ";
            }
            os << "> ELSE branch" << std::endl;
            print(F.E, os);
        }
    }

    /* Get the truth table of the BDD rooted at node f. */
    Truth_Table get_tt(signal_t f) const {
        Node const &F = get_node(f);

        if (f == constant(false)) {
            return Truth_Table(num_vars());
        } else if (f == constant(true)) {
            return ~Truth_Table(num_vars());
        }

        /* Shannon expansion: f = x f_x + x' f_x' */
        var_t const x = F.v;
        signal_t const fx = F.T;
        signal_t const fnx = F.E;
        Truth_Table const tt_x = create_tt_nth_var(num_vars(), x);
        Truth_Table const tt_nx = create_tt_nth_var(num_vars(), x, false);
        if (branch_mask(f)) {
            return ~((tt_x & get_tt(fx)) | (tt_nx & get_tt(fnx)));
        } else {
            return (tt_x & get_tt(fx)) | (tt_nx & get_tt(fnx));
        }
    }

    /* Whether `f` is dead (having a reference count of 0). */
    bool is_dead(index_t f) const {
        return refs[f] == 0u;
    }

    /* Get the number of living nodes in the whole package, excluding constants. */
    uint64_t num_nodes() const {
        uint64_t n = 0u;
        for (auto i = 1u; i < nodes.size(); ++i) {
            if (!is_dead(i)) {
                ++n;
            }
        }
        return n;
    }

    /* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
    uint64_t num_nodes(signal_t f) const {
        if (f == constant(false) || f == constant(true)) {
            return 0u;
        }

        std::vector<bool> visited(nodes.size(), false);
        visited[0] = true;

        return num_nodes_rec(get_index(f), visited);
    }

    uint64_t num_invoke() const {
        return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
    }

private:
    /**********************************************************/
    /******************** Helper Functions ********************/
    /**********************************************************/

    uint64_t num_nodes_rec(index_t f, std::vector<bool> &visited) const {
        assert(f < nodes.size() && "Make sure f exists.");

        uint64_t n = 0u;
        Node const &F = nodes[f];
        if (!visited[get_index(F.T)]) {
            n += num_nodes_rec(get_index(F.T), visited);
            visited[get_index(F.T)] = true;
        }
        if (!visited[get_index(F.E)]) {
            n += num_nodes_rec(get_index(F.E), visited);
            visited[get_index(F.E)] = true;
        }
        return n + 1u;
    }

private:
    std::vector<Node> nodes;
    std::vector<uint32_t> refs; /* The reference counts. Should always be the same size as `nodes`. */
    std::vector<std::unordered_map<std::pair<signal_t, signal_t>, index_t>> unique_table;
    /* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
     * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
     * See the implementation of `unique` for example usage. */

    /* Computed tables for each operation type. */
    std::unordered_map<binary_cache_key, signal_t> computed_table_AND;
    std::unordered_map<binary_cache_key, signal_t> computed_table_OR;
    std::unordered_map<binary_cache_key, signal_t> computed_table_XOR;
    std::unordered_map<ternary_cache_key, signal_t> computed_table_ITE;

    /* statistics */
    uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
};
