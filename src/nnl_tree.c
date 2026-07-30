#define NNL_TREE_DEF
#include "nnl_tree.h"

#include <utstack.h>

// Tree root address
const nnl_addr_t nnl_root = 0x80000000;

uint32_t nnl_tree_size( uint8_t scheme_depth)
{
	// assert( scheme_depth <= 8*sizeof(nnl_addr_t)-1 );
	return ( 1 << (scheme_depth+1) ) - 1;
}

uint32_t nnl_tree_offset( nnl_addr_t node_addr)
{
	uint32_t mask = 0xFFFFFFFF;
	uint8_t node_depth = 32;

	// assert( node_addr != 0 );
	while(!(node_addr & ~mask))
	{
		mask <<= 1;
		--node_depth;
	}

	// assert( node_depth <= scheme_depth );
	return nnl_tree_size( node_depth-1) + ( (node_addr & mask) >> (32-node_depth) );
}

int nnl_encode_uv( nnl_addr_t u, nnl_addr_t v, nnl_sd_t *sd)
{
	if( !u || !v || !sd )
		return 0;

	uint32_t u_mask = 0xFFFFFFFF;
	uint32_t v_mask = 0xFFFFFFFF;

	// Compute U mask and U_shift
	sd->u_shift = 0;
	while(!(u & ~u_mask))
	{
		u_mask <<= 1;
		sd->u_shift++;
	}

	// Compute V mask
	while(!(v & ~v_mask)) v_mask <<= 1;

	// Error if V isn't covered by U
	if( (u & u_mask ) != (v & u_mask) )
		return 0;

	// Store V in the UV value
	sd->uv = v;

	return 1;
}

int nnl_decode_uv( const nnl_sd_t *sd, nnl_addr_t *u, nnl_addr_t *v)
{
	if( !u || !v )
		return 0;

	nnl_addr_t u_mask = (nnl_addr_t)-1 << sd->u_shift;
	nnl_addr_t v_mask = (nnl_addr_t)-1;

	while(!(sd->uv & ~v_mask)) v_mask <<= 1;

	*u = (sd->uv & u_mask) | ( 1 << (sd->u_shift - 1) );
	*v = sd->uv;

	// Error if V isn't covered by U
	if( (*u & u_mask) != (*v & u_mask))
		return 0;

	return 1;
}

uint8_t nnl_depth( nnl_addr_t u)
{
	// assert( u );
	uint8_t depth = 32;
	uint32_t mask = 0xFFFFFFFF;
	while(!( u & ~mask ))
	{
		mask <<= 1;
		--depth;
	}
	return depth;
}

uint32_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth)
{
	// assert( scheme_depth < 8*sizeof(nnl_addr_t) );
	return 1 << ( scheme_depth - nnl_depth(u) );
}

nnl_state_t nnl_node_state( nnl_addr_t u, uint8_t scheme_depth, uint32_t *rvk_tree)
{
	uint32_t offset = nnl_tree_offset( u);
	if( !rvk_tree )
		return NNL_ST_REVOKED; // FIXME
	// assert( offset < (sizeof( rvk_tree ) / sizeof(uint32_t))) ?

	if( rvk_tree[offset] == 0 )
		return NNL_ST_VALID;

	if( rvk_tree[offset] == nnl_nb_leaves( u, scheme_depth) )
		return NNL_ST_REVOKED;

	return NNL_ST_MIXED;
}

/** [!PSEUDO-CODE]
cover_tree()
{
	nnl_addr_t cur, i, left_child, right_child, *stack = NULL;
	nnl_state_t cur_state, left_state, right_state;

	UT_icd ud_sd_ent_icd;
	UT_array *sd_tree;
	utarray_new( sd_tree, &ut_sd_end_icd);

	STACK_PUSH( stack, nnl_root);

	while(!STACK_EMPTY( stack))
	{
		STACK_POP( stack, cur);
		cur_state = nnl_node_state( cur, scheme_depth, rvk_tree);

		if( cur_state == NNL_ST_VALID )
		{
			emit( T[cur] ); // aes_g3( G_DIR_PROCESS, node_key( cur), sd->key ); utarray_push_back( sd_tree, sd);
			continue;
		}

		if( cur_state == NNL_ST_REVOKED )
			continue;

		// cur is MIXED. Descend until the revoked region branches
		i = cur;
		while(true)
		{
			left_child = nnl_left( i);
			left_state = nnl_node_state( left_child, scheme_depth, rvk_tree);

			right_child = nnl_right( i);
			right_state = nnl_node_state( right_child, scheme_depth, rvk_tree);

			if( left_state == NNL_ST_REVOKED && right_state == NNL_ST_REVOKED)
				break; // Safeguard

			if( left_state == NNL_ST_REVOKED )
			{
				emit_diff( i, left_child); // S{i,L} = T_i \ T_L
				// ??? aes_g3( G_DIR_LEFT, node_key( i), sd->key ); utarray_push_back( sd_tree, sd);
				i = right_child;
			}
			else if( right_state == NNL_ST_REVOKED )
			{
				emit_diff( i, right_child); // S{i,R} = T_i \ T_R
				// ??? aes_g3( G_DIR_RIGHT, node_key( i), sd->key ); utarray_push_back( sd_tree, sd);
				i = left_child;
			}
			else
			{
				// Both children are MIXED
				STACK_PUSH( stack, left_child);
				STACK_PUSH( stack, right_child);
				break;
			}

			if( nnl_node_state( i, scheme_depth, rvk_tree) == NNL_ST_VALID )
			{
				emit( T[i] ); // aes_g3( G_DIR_PROCESS, node_key( i), sd->key ); utarray_push_back( sd_tree, sd);
				break;
			}
		}
	}
}
 **/
