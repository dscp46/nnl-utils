#ifndef NNL_TREE_H
#define NNL_TREE_H

#include <stdint.h>
#include "nnl_types.h"

uint32_t nnl_tree_size( uint8_t scheme_depth);
uint32_t nnl_tree_offset( nnl_addr_t node_addr);
nnl_addr_t nnl_left( nnl_addr_t u);
nnl_addr_t nnl_right( nnl_addr_t u);
int nnl_encode_uv( nnl_addr_t u, nnl_addr_t v, nnl_sd_t *sd);
int nnl_decode_uv( const nnl_sd_t *sd, nnl_addr_t *u, nnl_addr_t *v);

uint8_t nnl_depth( nnl_addr_t u);
uint32_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth);
nnl_state_t nnl_node_state( nnl_addr_t u, uint8_t scheme_depth, const uint32_t *rvk_tree);

nnl_tree_t* nnl_tree_init( uint8_t scheme_depth, uint8_t partition_depth);
int nnl_tree_runtests( void);
#endif	/* NNL_TREE_H */
