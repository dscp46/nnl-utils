#ifndef NNL_TYPES_H
#define NNL_TYPES_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

// Node address (path to the leaf, followed by a terminal '1' bit).
typedef uint32_t nnl_addr_t;

#ifndef NNL_TREE_DEF
// Number of bits in a nnl_addr_t
extern const size_t NNL_ADDR_BITS;
// Tree root address
extern const nnl_addr_t nnl_root, nnl_invalid;
#endif	/* NNL_TREE_DEF */

// Subset-Difference UV structure
typedef struct nnl_sd nnl_sd_t;
struct nnl_sd {
	uint8_t u_shift;
	nnl_addr_t uv;
};

typedef enum {
	NNL_ST_VALID,
	NNL_ST_MIXED,
	NNL_ST_REVOKED
} nnl_state_t;

typedef struct nnl_tree nnl_tree_t;
struct nnl_tree {
	// Depth of the partition roots.
	uint8_t partition_depth;
	// Length of node addresses
	uint8_t scheme_depth;
	// Revocation tree
	uint32_t *rvk_tree;

	void (*free)( nnl_tree_t *self);
	void (*generate_sd_tree)( nnl_tree_t *self);
	void (*print_rvk)( nnl_tree_t *self);
	void (*revoke_node)( nnl_tree_t *self, nnl_addr_t addr);
};

#endif	/* NNL_TYPES_H */
