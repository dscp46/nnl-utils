#include "nnl_tree.h"

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


