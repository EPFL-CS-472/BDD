# BDD
 Stand-alone BDD package

**Diff will be large as the whole project is formatted with CLion!!!**

## Solution

### Expanded truth table

Truth tables are implemented such that they can deal with 2^32 variables. This is implemented using an unordered map
from a vector of boolean to the 64 bit table. The implementation is an extension such that 6 variables are directly
encoded into the keys of the map while the other possible 2^32 - 6 variables are used to build keys of the map.

A truth table with 6 variables the constant result is true would look like this:

() --> 0xffffffffffffffff

A truth table with 6 variables where the result is true when variable 0 is true would look like this:

() --> 0xaaaaaaaaaaaaaaaa

A truth table for 8 variables such that all value are 0 would look like this:

(false, false)  --> 0x0000000000000000

(false, true)  --> 0x0000000000000000

(true, false) --> 0x0000000000000000

(true, true) --> 0x0000000000000000

A truth table with 8 variables where the result is true when variable 7 is true would look like this:

(false, false)  --> 0x0000000000000000

(false, true)  --> 0xFFFFFFFFFFFFFFFF

(true, false) --> 0x0000000000000000

(true, true) --> 0xFFFFFFFFFFFFFFFF

### Operation cache

Operation cache is split in two parts:

- Cache selection
- Existence check

#### Cache selection

Selection the operation cache is done based on the functional equivalence of possibly previously computed operations.
This means that I am able to use laws of commutativity, De Morgan's laws, etc. to reduce the number of created nodes.
Cache search options are constructed inside methods AND, OR, etc. if there is a cache miss I the process the arguments
by calling AND_INTERNAL, OR_INTERNAL, etc. which are almost the original functions as in the skeleton.
As a result I can check if the following have been previously computed, for each operation:

- AND(f, g): AND(f, g), AND(g, f), OR(NOT(F), NOT(G)), OR(NOT(G), NOT(F))
- OR(f, g): OR(f, g), OR(g, f), AND(NOT(F), NOT(G)), AND(NOT(G), NOT(F))
- XOR(f, g): XOR(f, g), XOR(g, f), XOR(NOT(F), NOT(G)), XOR(NOT(G), NOT(F))
- ITE(f, g, h): ITE(f, g, h), ITE(NOT(f), h, g), ITE(f, NOT(g), NOT(h)), ITE(NOT(f), NOT(h), NOT(g)).

#### Existence check

Now that I have the options for each operation, I can iterate through them and return the edge pointing to the previously
computed node. In situations such as AND(f, g) == NOT(OR(NOT(f), NOT(g))), I also pass in a flag to tell us to complement
possibly precomputed OR(NOT(f), NOT(g)). Negating the previously computed result is analogous for all cases in which
functional equivalence is achieved by complementing existing nodes. If the cache is missed I try to construct a new
unique node and insert it into the correct cache. This logic is stored in `cached_computation`.

#### Complemented edges and uniqueness

**The solution is based on the hits commit.**
When a NOT operation is called I just invert the bit and return. To deal with this when processing the functions OR,
AND, etc. before the recursive call, when needed (when passing edges pointing to the children of the node pointed to by
a complemented edge), I invert the bit of the edges passed down to the recursive calls.

Uniqueness is achieved in the same manner as operation cache, we use the uniqueness table for a variable to check if a
node exists for node with children pointed to by passed in edges or complements of passed in edge. First case is trivial,
while in the second case, if such node already exists we return a complement to it (two complements will cancel each
other out when we actually compute). If the uniqueness table does not have a satisfactory result we construct a new node
and update the necessary data strictures/

#### Reference counting

Reference counting is implemented fairly simply. We keep the count of references pointing to each node and increase it
every time someone refs it or it is used as a child of a new node. When we dereference a node we decrease its' reference
count by one, if it reaches zero we call deref on each of its' children. This process continues recursively.

