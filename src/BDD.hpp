#pragma once

#include "truth_table.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

/* These are just some hacks to hash std::pair (for the unique table).
 * You don't need to understand this part. */
namespace std {
	template<class T>
	inline void hash_combine(size_t& seed, T const& v) {
		seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template<>
	struct hash<pair<uint32_t, uint32_t>>
	{
		using argument_type = pair<uint32_t, uint32_t>;
		using result_type = size_t;
		result_type operator() (argument_type const& in) const {
			result_type seed = 0;
			hash_combine(seed, in.first);
			hash_combine(seed, in.second);
			return seed;
		}
	};
}

class BDD
{
public:
	using index_t = uint32_t;
	/* Declaring `index_t` as an alias for an unsigned integer.
	 * This is just for easier understanding of the code.
	 * This datatype will be used for node indices. */

	using var_t = uint32_t;
	/* Similarly, declare `var_t` also as an alias for an unsigned integer.
	 * This datatype will be used for representing variables. */

private:
	typedef struct Comp_edge_t {
		index_t i;//Index of edge's child
		bool comp = false; //Is complemented

		bool operator==(Comp_edge_t& edge) {
			return this->i == edge.i && this->comp == edge.comp;
		}
	}comp_edge_t;

	typedef struct node
	{
		var_t v; /* corresponding variable */
		comp_edge_t T; /* index of THEN child */
		comp_edge_t E; /* index of ELSE child */
		uint32_t ref_count = 0; //Reference count of node
	}Node;

	typedef struct nodeCache {
		std::string op;
		index_t result;
		index_t node1;
		index_t node2 = UINT32_MAX;
		index_t node3 = UINT32_MAX;
	}NodeCache;

public:
	explicit BDD(uint32_t num_vars)
		: unique_table(num_vars), num_invoke_and(0u), num_invoke_or(0u),
		num_invoke_xor(0u), num_invoke_ite(0u) {
		nodes.emplace_back(Node({ num_vars, comp_edge_t{0}, comp_edge_t{0} })); /* constant 0 */
		nodes.emplace_back(Node({ num_vars, comp_edge_t{1}, comp_edge_t{1} })); /* constant 1 */
		/* `nodes` is initialized with two `Node`s representing the terminal (constant) nodes.
		 * Their `v` is `num_vars` and their indices are 0 and 1.
		 * (Note that the real variables range from 0 to `num_vars - 1`.)
		 * Both of their children point to themselves, just for convenient representation.
		 *
		 * `unique_table` is initialized with `num_vars` empty maps. */
	}

	/**********************************************************/
	/***************** Basic Building Blocks ******************/
	/**********************************************************/

	uint32_t num_vars() const {
		return unique_table.size();
	}

	/* Get the (index of) constant node. */
	index_t constant(bool value) const {
		return value ? 1 : 0;
	}

	/* Look up (if exist) or build (if not) the node with variable `var`,
	 * THEN child `T`, and ELSE child `E`. */
	index_t unique(var_t var, comp_edge_t T, comp_edge_t E) {
		assert(var < num_vars() && "Variables range from 0 to `num_vars - 1`.");
		assert(T.i < nodes.size() && "Make sure the children exist.");
		assert(E.i < nodes.size() && "Make sure the children exist.");
		assert(nodes[T.i].v > var && "With static variable order, children can only be below the node.");
		assert(nodes[E.i].v > var && "With static variable order, children can only be below the node.");

		/* Reduction rule: Identical children */
		if (T == E) {
			return T.i;
		}

		/* Look up in the unique table. */
		const auto it = unique_table[var].find({ T.i, E.i });
		if (it != unique_table[var].end()) {
			/* The required node already exists. Return it. */
			return it->second;
		}
		else {
			/* Create a new node and insert it to the unique table. */
			index_t const new_index = nodes.size();
			nodes.emplace_back(Node({ var, T, E }));
			unique_table[var][{T.i, E.i}] = new_index;
			ref(T.i); //nodes[T.i].ref_count++;
			ref(E.i); //nodes[E.i].ref_count++;
			return new_index;
		}
	}

	/* Return a node (represented with its index) of function F = x_var or F = ~x_var. */
	index_t literal(var_t var, bool complement = false) {
		return unique(var, comp_edge_t{ constant(!complement) }, comp_edge_t{ constant(complement) });
	}

	/**********************************************************/
	/********************* BDD Operations *********************/
	/**********************************************************/

	std::vector<NodeCache> cache{};
	index_t nodeInCache(std::string op, index_t node1, index_t node2 = UINT32_MAX, index_t node3 = UINT32_MAX) {
		for (auto& node : cache) {
			//1 node
			if (node2 == UINT32_MAX) {
				if (node.op == op && node.node1 == node1)
					return node.result;
			}
			//2 nodes
			if (node3 == UINT32_MAX)
				if (node.op == op && node1 == node.node1 && node2 == node.node2)
					return node.result;
			//3 nodes
			if (node.op == op &&
				node1 == node.node1 && node2 == node.node2 && node3 == node.node3)
				return node.result;
		}
		return UINT32_MAX;
	}

	/* Compute ~f */
	index_t NOT(index_t f) {
		assert(f < nodes.size() && "Make sure f exists.");
		index_t cacheNode = nodeInCache("NOT", f);
		if (cacheNode != UINT32_MAX) {
			return cacheNode;
		}

		/* trivial cases */
		if (f == constant(false)) {
			return constant(true);
		}
		if (f == constant(true)) {
			return constant(false);
		}

		Node const& F = nodes[f];
		var_t x = F.v;

		index_t resultNode = unique(x, comp_edge_t{ F.E.i }, comp_edge_t{ F.T.i });
		cache.push_back(NodeCache{ "XOR", resultNode, f });
		return resultNode;
	}

	/* Compute f ^ g */
	index_t XOR(index_t f, index_t g) {
		assert(f < nodes.size() && "Make sure f exists.");
		assert(g < nodes.size() && "Make sure g exists.");
		++num_invoke_xor;
		index_t cacheNode = nodeInCache("XOR", f, g);
		if (cacheNode != UINT32_MAX) {
			return cacheNode;
		}

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

		Node const& F = nodes[f];
		Node const& G = nodes[g];

		var_t x;
		index_t f0, f1, g0, g1;
		if (F.v < G.v) /* F is on top of G */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = g1 = g;
		}
		else if (G.v < F.v) /* G is on top of F */
		{
			x = G.v;
			f0 = f1 = f;
			g0 = G.E.i;
			g1 = G.T.i;
		}
		else /* F and G are at the same level */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = G.E.i;
			g1 = G.T.i;
		}

		comp_edge_t const r0 = comp_edge_t{ XOR(f0, g0) };
		comp_edge_t const r1 = comp_edge_t{ r0.i, true };

		index_t resultNode = unique(x, r1, r0);
		cache.push_back(NodeCache{ "XOR", resultNode, f, g });
		return resultNode;
	}



	/* Compute f & g */
	index_t AND(index_t f, index_t g) {
		assert(f < nodes.size() && "Make sure f exists.");
		assert(g < nodes.size() && "Make sure g exists.");
		++num_invoke_and;
		index_t cacheNode = nodeInCache("AND", f, g);
		if (cacheNode != UINT32_MAX) {
			return cacheNode;
		}

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

		Node const& F = nodes[f];
		Node const& G = nodes[g];
		var_t x;
		index_t f0, f1, g0, g1;
		if (F.v < G.v) /* F is on top of G */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = g1 = g;
		}
		else if (G.v < F.v) /* G is on top of F */
		{
			x = G.v;
			f0 = f1 = f;
			g0 = G.E.i;
			g1 = G.T.i;
		}
		else /* F and G are at the same level */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = G.E.i;
			g1 = G.T.i;
		}

		comp_edge_t const r0 = comp_edge_t{ AND(F.E.comp ? NOT(nodes[f0].E.i) : f0, G.E.comp ? NOT(nodes[g0].E.i) : g0) };
		comp_edge_t const r1 = comp_edge_t{ AND(F.T.comp ? NOT(nodes[f1].T.i) : f1, G.T.comp ? NOT(nodes[g1].T.i) : g1) };
		index_t resultNode = unique(x, r1, r0);
		cache.push_back(NodeCache{ "AND", resultNode, f, g });
		return resultNode;
	}

	/* Compute f | g */
	index_t OR(index_t f, index_t g) {
		assert(f < nodes.size() && "Make sure f exists.");
		assert(g < nodes.size() && "Make sure g exists.");
		++num_invoke_or;
		index_t cacheNode = nodeInCache("OR", f, g);
		if (cacheNode != UINT32_MAX) {
			return cacheNode;
		}

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

		Node const& F = nodes[f];
		Node const& G = nodes[g];
		var_t x;
		index_t f0, f1, g0, g1;
		if (F.v < G.v) /* F is on top of G */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = g1 = g;
		}
		else if (G.v < F.v) /* G is on top of F */
		{
			x = G.v;
			f0 = f1 = f;
			g0 = G.E.i;
			g1 = G.T.i;
		}
		else /* F and G are at the same level */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			g0 = G.E.i;
			g1 = G.T.i;
		}

		comp_edge_t const r0 = comp_edge_t{ OR(F.E.comp ? NOT(nodes[f0].E.i) : f0, G.E.comp ? NOT(nodes[g0].E.i) : g0) };
		comp_edge_t const r1 = comp_edge_t{ OR(F.T.comp ? NOT(nodes[f1].T.i) : f1, G.T.comp ? NOT(nodes[g1].T.i) : g1) };
		index_t resultNode = unique(x, r1, r0);
		cache.push_back(NodeCache{ "OR", resultNode, f, g });
		return resultNode;
	}

	/* Compute ITE(f, g, h), i.e., f ? g : h */
	index_t ITE(index_t f, index_t g, index_t h) {
		assert(f < nodes.size() && "Make sure f exists.");
		assert(g < nodes.size() && "Make sure g exists.");
		assert(h < nodes.size() && "Make sure h exists.");
		++num_invoke_ite;
		index_t cacheNode = nodeInCache("ITE", f, g, h);
		if (cacheNode != UINT32_MAX) {
			return cacheNode;
		}

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

		Node const& F = nodes[f];
		Node const& G = nodes[g];
		Node const& H = nodes[h];
		var_t x;
		index_t f0, f1, g0, g1, h0, h1;
		if (F.v <= G.v && F.v <= H.v) /* F is not lower than both G and H */
		{
			x = F.v;
			f0 = F.E.i;
			f1 = F.T.i;
			if (G.v == F.v) {
				g0 = G.E.i;
				g1 = G.T.i;
			}
			else {
				g0 = g1 = g;
			}
			if (H.v == F.v) {
				h0 = H.E.i;
				h1 = H.T.i;
			}
			else {
				h0 = h1 = h;
			}
		}
		else /* F.v > min(G.v, H.v) */
		{
			f0 = f1 = f;
			if (G.v < H.v) {
				x = G.v;
				g0 = G.E.i;
				g1 = G.T.i;
				h0 = h1 = h;
			}
			else if (H.v < G.v) {
				x = H.v;
				g0 = g1 = g;
				h0 = H.E.i;
				h1 = H.T.i;
			}
			else /* G.v == H.v */
			{
				x = G.v;
				g0 = G.E.i;
				g1 = G.T.i;
				h0 = H.E.i;
				h1 = H.T.i;
			}
		}

		comp_edge_t const r0 = comp_edge_t{ ITE(f0, g0, h0) };
		comp_edge_t const r1 = comp_edge_t{ ITE(f1, g1, h1) };
		index_t resultNode = unique(x, r1, r0);
		cache.push_back(NodeCache{ "ITE", resultNode, f, g, h });
		return resultNode;
	}

	index_t ref(index_t f) {
		nodes[f].ref_count++;
		return f;
	}

	void deref(index_t f) {
		nodes[f].ref_count--;
		if (is_dead(f)) {
			deref(nodes[f].T.i);
			deref(nodes[f].E.i);
		}
	}

	/**********************************************************/
	/***************** Printing and Evaluating ****************/
	/**********************************************************/

	/* Print the BDD rooted at node `f`. */
	void print(index_t f, std::ostream& os = std::cout) const {
		for (auto i = 0u; i < nodes[f].v; ++i) {
			os << "  ";
		}
		if (f <= 1) {
			os << "node " << f << ": constant " << f << std::endl;
		}
		else {
			os << "node " << f << ": var = " << nodes[f].v << ", T = " << nodes[f].T.i
				<< ", E = " << nodes[f].E.i << std::endl;
			for (auto i = 0u; i < nodes[f].v; ++i) {
				os << "  ";
			}
			os << "> THEN branch" << std::endl;
			print(nodes[f].T.i, os);
			for (auto i = 0u; i < nodes[f].v; ++i) {
				os << "  ";
			}
			os << "> ELSE branch" << std::endl;
			print(nodes[f].E.i, os);
		}
	}

	/* Get the truth table of the BDD rooted at node f. */
	Truth_Table get_tt(index_t f) const {
		assert(f < nodes.size() && "Make sure f exists.");
		//assert(num_vars() <= 6 && "Truth_Table only supports functions of no greater than 6 variables.");

		if (f == constant(false)) {
			return Truth_Table(num_vars());
		}
		else if (f == constant(true)) {
			return ~Truth_Table(num_vars());
		}

		/* Shannon expansion: f = x f_x + x' f_x' */
		var_t const x = nodes[f].v;
		comp_edge_t const fx = nodes[f].T;
		comp_edge_t const fnx = nodes[f].E;
		Truth_Table const tt_x = create_tt_nth_var(num_vars(), x);
		Truth_Table const tt_nx = create_tt_nth_var(num_vars(), x, false);
		return (tt_x & (fx.comp ? ~get_tt(fx.i) : get_tt(fx.i))) | (tt_nx & (fnx.comp ? ~get_tt(fnx.i) : get_tt(fnx.i)));
	}

	/* Whether `f` is dead (having a reference count of 0). */
	bool is_dead(index_t f) const {
		return nodes[f].ref_count <= 0;
	}

	/* Get the number of living nodes in the whole package, excluding constants. */
	uint64_t num_nodes() const {
		uint64_t n = 0u;
		for (auto i = 2u; i < nodes.size(); ++i) {
			if (!is_dead(i)) {
				++n;
			}
		}
		return n;
	}

	/* Get the number of nodes in the sub-graph rooted at node f, excluding constants. */
	uint64_t num_nodes(index_t f) const {
		assert(f < nodes.size() && "Make sure f exists.");

		if (f == constant(false) || f == constant(true)) {
			return 0u;
		}

		std::vector<bool> visited(nodes.size(), false);
		visited[0] = true;
		visited[1] = true;

		return num_nodes_rec(f, visited);
	}

	uint64_t num_invoke() const {
		return num_invoke_and + num_invoke_or + num_invoke_xor + num_invoke_ite;
	}

private:
	/**********************************************************/
	/******************** Helper Functions ********************/
	/**********************************************************/

	uint64_t num_nodes_rec(index_t f, std::vector<bool>& visited) const {
		assert(f < nodes.size() && "Make sure f exists.");


		uint64_t n = 0u;
		Node const& F = nodes[f];
		assert(F.T.i < nodes.size() && "Make sure the children exist.");
		assert(F.E.i < nodes.size() && "Make sure the children exist.");
		if (!visited[F.T.i]) {
			n += num_nodes_rec(F.T.i, visited);
			visited[F.T.i] = true;
		}
		if (!visited[F.E.i]) {
			n += num_nodes_rec(F.E.i, visited);
			visited[F.E.i] = true;
		}
		return n + 1u;
	}

private:
	std::vector<Node> nodes;
	std::vector<std::unordered_map<std::pair<index_t, index_t>, index_t>> unique_table;
	/* `unique_table` is a vector of `num_vars` maps storing the built nodes of each variable.
	 * Each map maps from a pair of node indices (T, E) to a node index, if it exists.
	 * See the implementation of `unique` for example usage. */

	 /* statistics */
	uint64_t num_invoke_and, num_invoke_or, num_invoke_xor, num_invoke_ite;
};
