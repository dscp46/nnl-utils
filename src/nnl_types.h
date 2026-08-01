#ifndef NNL_TYPES_H
#define NNL_TYPES_H

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

#endif	/* NNL_TYPES_H */
