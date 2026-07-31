#ifndef NNL_TREE_H
#define NNL_TREE_H

#include <stdint.h>

// Node address (path to the leaf, followed by a terminal '1' bit).
typedef uint32_t nnl_addr_t;

#ifndef NNL_TREE_DEF
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

uint32_t nnl_tree_size( uint8_t scheme_depth);
uint32_t nnl_tree_offset( nnl_addr_t node_addr);
int nnl_encode_uv( nnl_addr_t u, nnl_addr_t v, nnl_sd_t *sd);
int nnl_decode_uv( const nnl_sd_t *sd, nnl_addr_t *u, nnl_addr_t *v);

uint8_t nnl_depth( nnl_addr_t u);
uint32_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth);
nnl_state_t nnl_node_state( nnl_addr_t u, uint8_t scheme_depth, const uint32_t *rvk_tree);

#endif	/* NNL_TREE_H */
