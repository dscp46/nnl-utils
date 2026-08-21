#ifndef NNL_TREE_H
#define NNL_TREE_H

#include <stdint.h>
#include "nnl_ops.h"
#include "nnl_types.h"
#include "hsm.h"

typedef struct nnl_device_key nnl_dk_t;
struct nnl_device_key {
	nnl_sd_t uv;
	CK_BYTE *devkey;
	struct nnl_device_key *next;
	void (*free)( struct nnl_device_key *self);
	void (*serialize)( struct nnl_device_key *self, CK_BYTE dkek[]);
};

size_t nnl_tree_offset( nnl_addr_t node_addr, const nnl_tree_t *tree);

int nnl_encode_uv( nnl_addr_t u, nnl_addr_t v, nnl_sd_t *sd);
int nnl_decode_uv( const nnl_sd_t *sd, nnl_addr_t *u, nnl_addr_t *v);

nnl_state_t nnl_node_state( nnl_addr_t u, const nnl_tree_t *self);

int nnl_emit_device_keys( hsm_t *hsm, nnl_tree_t *tree, nnl_addr_t addr, size_t key_len, nnl_dk_t **dk_list); 

nnl_tree_t* nnl_tree_init( uint8_t scheme_depth, uint8_t partition_depth);
int nnl_tree_runtests( void);
#endif	/* NNL_TREE_H */
