#define NNL_CONSTS_DEF
#include "nnl_ops.h"

const size_t NNL_ADDR_BITS = CHAR_BIT * sizeof( nnl_addr_t);

// Tree root address
const nnl_addr_t nnl_root    = (nnl_addr_t)1 << (NNL_ADDR_BITS-1);

// Invalid address
const nnl_addr_t nnl_invalid = (nnl_addr_t)0;

nnl_addr_t nnl_addr( size_t path, uint8_t depth)
{
	// We need one sentinel bit below the path.
	if( depth > NNL_ADDR_BITS-1 )
		return nnl_invalid;

	nnl_addr_t addr = (path << 1) | 1; // Path followed by the sentinel bit

	// Shift the path's MSB to addr's MSB
	for( size_t i=0; i<(NNL_ADDR_BITS-depth-1); ++i)
		addr <<= 1;

	return addr;
}

void nnl_build_mask( nnl_addr_t addr, nnl_addr_t *mask, uint8_t *depth, uint8_t *shift)
{
	if( !addr || !mask )
		return;

	*mask = (nnl_addr_t) -1;

	if( depth )
		*depth = NNL_ADDR_BITS;

	if( shift )
		*shift = 0;

	// assert( addr != nnl_invalid );
	while(!(addr & ~*mask))
	{
		*mask <<= 1;

		if( depth )
			--(*depth);

		if( shift )
			++(*shift);
	}
}

size_t nnl_tree_size( uint8_t scheme_depth, uint8_t partition_depth)
{
	// assert( scheme_depth <= 8*sizeof(nnl_addr_t)-1 );
	// 2^{p} * ( 2^{d-p+1} - 1)
	size_t result = ( (size_t)1 << (scheme_depth-partition_depth+1) ) - 1;
	result *= (size_t)1 << partition_depth;
	return result;
}

inline nnl_addr_t nnl_left( nnl_addr_t u)
{
	if( u == nnl_invalid)
		return nnl_invalid;

	uint8_t shift;
	nnl_addr_t mask;
	nnl_build_mask( u, &mask, NULL, &shift);

	u &= ~( (nnl_addr_t)1 << (shift-1) ); // Clear child addr
	u |= (nnl_addr_t)1 << (shift-2) ; // Set mask bit
	return u;
}

inline nnl_addr_t nnl_right( nnl_addr_t u)
{
	if( u == nnl_invalid)
		return nnl_invalid;

	uint8_t shift;
	nnl_addr_t mask;
	nnl_build_mask( u, &mask, NULL, &shift);

	u |= (nnl_addr_t)3 << (shift-2) ; // Set child and mask bits
	return u;
}

inline nnl_addr_t nnl_parent( nnl_addr_t u)
{
	if( u == nnl_invalid)
		return nnl_invalid;

	uint8_t shift;
	nnl_addr_t mask;
	nnl_build_mask( u, &mask, NULL, &shift);

	u |= 1 << shift;
	u &= mask;

	return u;
}

inline nnl_addr_t nnl_opposite_branch( nnl_addr_t u)
{
	if( u == nnl_invalid)
		return nnl_invalid;

	uint8_t shift;
	nnl_addr_t mask, bit_mask;
	nnl_build_mask( u, &mask, NULL, &shift);

	bit_mask = mask & ~(mask << 1); // Select last bit of the path
	return u ^ bit_mask; // Flip the last path bit
}

int nnl_is_parent( nnl_addr_t u, nnl_addr_t v)
{
	nnl_addr_t u_mask, v_mask;
	if( !u || !v )
		return 0;

	nnl_build_mask( u, &u_mask, NULL, NULL);
	nnl_build_mask( v, &v_mask, NULL, NULL);
	return ((u & u_mask) == (v & u_mask)) && (u_mask < v_mask);
}


uint8_t nnl_depth( nnl_addr_t u)
{
	// assert( u != nnl_invalid );
	uint8_t depth;
	nnl_addr_t mask;
	nnl_build_mask( u, &mask, &depth, NULL);

	return depth;
}

size_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth)
{
	// assert( scheme_depth < 8*sizeof(nnl_addr_t) );
	return (size_t)1 << ( scheme_depth - nnl_depth(u) );
}

