#ifndef NNL_OPS_H
#define NNL_OPS_H

#include <stdint.h>
#include "nnl_types.h"

void nnl_build_mask( nnl_addr_t addr, nnl_addr_t *mask, uint8_t *depth, uint8_t *shift);

size_t nnl_tree_size( uint8_t scheme_depth, uint8_t partition_depth);
nnl_addr_t nnl_left( nnl_addr_t u);
nnl_addr_t nnl_right( nnl_addr_t u);
nnl_addr_t nnl_parent( nnl_addr_t u);
nnl_addr_t nnl_opposite_branch( nnl_addr_t u);
int nnl_is_parent( nnl_addr_t u, nnl_addr_t v);

uint8_t nnl_depth( nnl_addr_t u);
size_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth);

int nnl_ops_runtests( void);

#endif	/* NNL_OPS_H */
