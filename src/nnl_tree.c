#define NNL_TREE_DEF
#include "nnl_tree.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utarray.h>
#include <utstack.h>

#include "addr_item.h"

// Tree root address
const nnl_addr_t nnl_root    = 0x80000000;

// Invalid address
const nnl_addr_t nnl_invalid = 0x00000000;

// Function definitions
static void nnl_tree_free( nnl_tree_t *tree);
static void nnl_tree_generate_sd_tree( nnl_tree_t *self);
static void nnl_tree_revoke_node( nnl_tree_t *self, nnl_addr_t addr);

inline static void nnl_build_mask( nnl_addr_t addr, uint32_t *mask, uint8_t *depth, uint8_t *shift)
{
	if( !addr || !mask )
		return;

	*mask = (nnl_addr_t) -1;

	if( depth )
		*depth = 32; // FIXME: base on nnl_addr_t bit size?

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

uint32_t nnl_tree_size( uint8_t scheme_depth)
{
	// assert( scheme_depth <= 8*sizeof(nnl_addr_t)-1 );
	return ( (uint32_t)1 << (scheme_depth+1) ) - 1;
}

uint32_t nnl_tree_offset( nnl_addr_t node_addr)
{
	uint32_t mask;
	uint8_t node_depth;
	nnl_build_mask( node_addr, &mask, &node_depth, NULL);

	// assert( node_depth <= scheme_depth );
	return nnl_tree_size( node_depth-1) + ( (node_addr & mask) >> (32-node_depth) );
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

int nnl_encode_uv( nnl_addr_t u, nnl_addr_t v, nnl_sd_t *sd)
{
	if( !u || !v || !sd )
		return 0;

	uint32_t u_mask, v_mask;
	nnl_build_mask( u, &u_mask, NULL, &(sd->u_shift));
	nnl_build_mask( v, &v_mask, NULL, NULL);

	// Error if V isn't covered by U
	if( (u & u_mask ) != (v & u_mask) ) // && u_mask <= v_mask ) ?
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
	nnl_build_mask( sd->uv, &v_mask, NULL, NULL);

	*u = (sd->uv & u_mask) | ( (nnl_addr_t)1 << (sd->u_shift - 1) );
	*v = sd->uv;

	// Error if V isn't covered by U
	if( (*u & u_mask) != (*v & u_mask))
		return 0;

	return 1;
}

uint8_t nnl_depth( nnl_addr_t u)
{
	// assert( u != nnl_invalid );
	uint8_t depth;
	uint32_t mask;
	nnl_build_mask( u, &mask, &depth, NULL);

	return depth;
}

uint32_t nnl_nb_leaves( nnl_addr_t u, uint8_t scheme_depth)
{
	// assert( scheme_depth < 8*sizeof(nnl_addr_t) );
	return (uint32_t)1 << ( scheme_depth - nnl_depth(u) );
}

nnl_state_t nnl_node_state( nnl_addr_t u, uint8_t scheme_depth, const uint32_t *rvk_tree)
{
	uint32_t offset = nnl_tree_offset( u);
	if( !rvk_tree )
		return NNL_ST_REVOKED; // FIXME
	// assert( rvk_tree && (offset < (sizeof( rvk_tree ) / sizeof(uint32_t)))) ?

	//printf( "state(%08" PRIx32 "): %s\n", u, (rvk_tree[offset] == 0)? "VALID" : (( rvk_tree[offset] == nnl_nb_leaves( u, scheme_depth) ) ? "REVOKED":"MIXED"));
	if( rvk_tree[offset] == 0 )
		return NNL_ST_VALID;

	if( rvk_tree[offset] == nnl_nb_leaves( u, scheme_depth) )
		return NNL_ST_REVOKED;

	return NNL_ST_MIXED;
}

static void nnl_tree_print_rvk( nnl_tree_t *self)
{
	if( !self || !self->rvk_tree )
	{
		printf( "[nnl_tree_print_rvk()] NULL pointer somewhere, nothing to print.\n");
		return;
	}

	uint8_t cur_depth = self->partition_depth;
	uint32_t nb_nodes = nnl_tree_size( self->scheme_depth-self->partition_depth);
	uint32_t nodes_in_layer = 1;
	printf( "%u: ", self->partition_depth);
	for( uint32_t i=0, j=0; i<nb_nodes; ++i)
	{
		++j;
		printf( "%" PRIu32 " ", self->rvk_tree[i]);

		if( j == nodes_in_layer )
		{
			++cur_depth;
			if( cur_depth <= self->scheme_depth )
				printf("\n%u: ", cur_depth);
			j=0;
			nodes_in_layer <<= 1;
		}
	}
	printf( "\n");
}

static void nnl_tree_revoke_node( nnl_tree_t *self, nnl_addr_t addr)
{
	if( !self || addr == nnl_invalid )
		return;
	uint8_t depth, shift;
	uint32_t mask;
	nnl_build_mask( addr, &mask, &depth, &shift);

	if( depth != self->scheme_depth )
		return;

	do
	{
		++(self->rvk_tree[ nnl_tree_offset( addr) ]);
		mask <<=1;
		addr &= mask;
		addr |= (nnl_addr_t)1 << shift++;
	}
	while( mask );
	
	// Increment root
	++(self->rvk_tree[0]);
}

static void nnl_tree_generate_sd_tree( nnl_tree_t *self)
{
	if( !self )
		return;

	nnl_sd_t uv;
	nnl_addr_t cur, i, left_child, right_child;
	nnl_state_t cur_state, left_state, right_state;

	/*
	UT_icd ut_sd_ent_icd;
	UT_array *sd_tree;
	utarray_new( sd_tree, &ut_sd_ent_icd);
	// */
	addr_item_t *stack = NULL, *cur_it;

	STACK_PUSH( stack, addr_item_new( nnl_root, stack));

	while(!STACK_EMPTY( stack))
	{
		STACK_POP( stack, cur_it);
		cur = cur_it->value;
		free( cur_it);

		if( nnl_depth( cur) >= self->scheme_depth )
			// Do not process leaves
			continue;

		cur_state = nnl_node_state( cur, self->scheme_depth, self->rvk_tree);

		if( cur_state == NNL_ST_VALID )
		{
			// TODO: special case: root and valid => emit T{r} \ Ø if associated Dk is issued
			continue;
		}

		if( cur_state == NNL_ST_REVOKED )
		{
			// Nothing to emit. A client will end up on the final UV with a revoked flag.
			continue;
		}

		i = cur;
		while(1)
		{
			left_child = nnl_left( i);
			left_state = nnl_node_state( left_child, self->scheme_depth, self->rvk_tree);

			right_child = nnl_right( i);
			right_state = nnl_node_state( right_child, self->scheme_depth, self->rvk_tree);

			if( left_state == NNL_ST_REVOKED && right_state == NNL_ST_REVOKED)
				break; // Safeguard, theorically unreachable

			if( left_state == NNL_ST_VALID )
			{
				if( right_state == NNL_ST_REVOKED )
				{
					nnl_encode_uv( cur, right_child, &uv);
					printf( "Emit T[%08"PRIx32"] \\ T[%08"PRIx32"] AKA T%2u\\T%2u. UV = %08"PRIx32", U_shift = 0x%02x\n", 
						cur, right_child, 
						nnl_tree_offset( cur)+1, nnl_tree_offset( right_child)+1,
						uv.uv, uv.u_shift
					);
					//emit_diff( i, right_child); // S{i,R} = T_i \ T_R
					// ??? aes_g3( G_DIR_RIGHT, node_key( i), sd->key ); utarray_push_back( sd_tree, sd);
					break;
				}
				// Right is mixed, go right
				i = right_child;
				//printf( "Branch right (%08"PRIx32").\n", i);
				continue;
			}
			else if( right_state == NNL_ST_VALID )
			{
				if( left_state == NNL_ST_REVOKED )
				{
					nnl_encode_uv( cur, left_child, &uv);
					printf( "Emit T[%08"PRIx32"] \\ T[%08"PRIx32"] AKA T%2u\\T%2u. UV = %08"PRIx32", U_shift = 0x%02x\n", 
						cur, left_child, 
						nnl_tree_offset( cur)+1, nnl_tree_offset( left_child)+1,
						uv.uv, uv.u_shift
					);
					//emit_diff( i, left_child); // S{i,L} = T_i \ T_L
					// ??? aes_g3( G_DIR_LEFT, node_key( i), sd->key ); utarray_push_back( sd_tree, sd);
					break;
				}
				// Left is mixed, go right
				i = left_child;
				//printf( "Branch left. (%08"PRIx32")\n", i);
				continue;
			}

			// Mixed on one side, revoked on the other: act as if fully revoked, and cover the mixed branch.
			else if( left_state == NNL_ST_MIXED && right_state == NNL_ST_REVOKED )
			{
				printf( "Emit T[%08"PRIx32"] \\ T[%08"PRIx32"] AKA T%2u\\T%2u. UV = %08"PRIx32", U_shift = 0x%02x\n", 
					cur, i,
					nnl_tree_offset( cur)+1, nnl_tree_offset( i)+1,
					uv.uv, uv.u_shift
				);
				STACK_PUSH( stack, addr_item_new( left_child, stack));
				break;
			}
			else if( left_state == NNL_ST_REVOKED && right_state == NNL_ST_MIXED )
			{
				printf( "Emit T[%08"PRIx32"] \\ T[%08"PRIx32"] AKA T%2u\\T%2u. UV = %08"PRIx32", U_shift = 0x%02x\n", 
					cur, i,
					nnl_tree_offset( cur)+1, nnl_tree_offset( i)+1,
					uv.uv, uv.u_shift
				);
				STACK_PUSH( stack, addr_item_new( right_child, stack));
				break;
			}

			// Subset-difference not applicable
			//printf( "SD not applicable, go 1 level down.\n");
			STACK_PUSH( stack, addr_item_new( right_child, stack));
			STACK_PUSH( stack, addr_item_new( left_child, stack));
			break;
		}
	}
	printf( "Emit Tr[partition_root], u_shift |= 0xC0\n");
	printf("done\n");
	// TODO: final stack cleanup to free leftovers in the stack
}

nnl_tree_t* nnl_tree_init( uint8_t scheme_depth, uint8_t partition_depth)
{
	if( scheme_depth < 2 || scheme_depth >= 31 || (partition_depth >= (scheme_depth-2)) )
		return NULL;

	nnl_tree_t *instance = (nnl_tree_t*) malloc( sizeof( nnl_tree_t));
	if( !instance )
		return NULL;

	//TODO: add a warning when scheme_depth-partition_depth exceeds a resource threshold

	instance->partition_depth = partition_depth;
	instance->scheme_depth = scheme_depth;
	instance->rvk_tree = calloc( sizeof( uint32_t), nnl_tree_size( scheme_depth-partition_depth));

	instance->free = nnl_tree_free;
	instance->generate_sd_tree = nnl_tree_generate_sd_tree;
	instance->print_rvk = nnl_tree_print_rvk;
	instance->revoke_node = nnl_tree_revoke_node;

	return instance;
}

static void nnl_tree_free( nnl_tree_t *self)
{
	if( !self )
		return;

	if( self->rvk_tree )
		free( self->rvk_tree );

	free( self);
}

int nnl_tree_runtests( void)
{
	printf( "Tree size (3): %" PRIu32 "\n", nnl_tree_size( 3));
	printf( "Offset( 101/3): %" PRIu32 "\n", nnl_tree_offset(0xB0000000));

	nnl_addr_t u = 0xB0000000;
	nnl_addr_t v = 0xA2D80000;
	nnl_addr_t u_prime, v_prime;
	nnl_sd_t uv;

	if( nnl_left( nnl_root) != ( (nnl_addr_t)1 << ((8*sizeof(nnl_addr_t)-2)) ) )
	{
		fprintf( stderr, 
			"nnl_left( root) failed. Got %" PRIx32 ", expected %" PRIx32 "\n",
			nnl_left( nnl_root),
		       (nnl_addr_t)1 << ((8*sizeof(nnl_addr_t)-2))
		);
		return 0;
	}

	if( nnl_right( nnl_root) != ( (nnl_addr_t)3 << ((8*sizeof(nnl_addr_t)-2)) ) )
	{
		fprintf( stderr, 
			"nnl_left( root) failed. Got %" PRIx32 ", expected %" PRIx32 "\n",
			nnl_left( nnl_root),
		       (nnl_addr_t)3 << ((8*sizeof(nnl_addr_t)-2))
		);
		return 0;
	}
	
	if(!nnl_encode_uv( u, v, &uv))
	{
		fprintf( stderr, "Failed to encode UV for U: 0x%" PRIx32 ", V: 0x%" PRIx32 "\n", u, v);
		return 0;
	}

	if(!nnl_decode_uv( &uv, &u_prime, &v_prime))
	{
		fprintf( stderr, "Failed to decode UV for UV: 0x%" PRIx32 ", U_shift: 0x%x\n", uv.uv, uv.u_shift);
		return 0;
	}

	if( u != u_prime || v != v_prime )
	{
		fprintf( stderr, "U (0x%" PRIx32") != U' (0x%"PRIx32")\n", u, u_prime);
		return 0;
	}

	if( v != v_prime )
	{
		fprintf( stderr, "V (0x%" PRIx32") != V' (0x%"PRIx32")\n", v, v_prime);
		return 0;
	}

	printf( "UV: 0x%" PRIx32 ", U_shift: 0x%x\n", uv.uv, uv.u_shift);

	nnl_tree_t *tree = nnl_tree_init( 5, 0);
	if( !tree )
	{
		printf( "Unable to allocate the toy tree");
		return 0;
	}

	tree->revoke_node( tree, 0x04000000); // Leaf 00000
	tree->print_rvk( tree);
	tree->generate_sd_tree( tree);
	tree->free( tree);


	tree = nnl_tree_init( 5, 0);
	if( !tree )
	{
		printf( "Unable to allocate the toy tree");
		return 0;
	}
	tree->revoke_node( tree, 0x4C000000); // Leaf 01001
	tree->print_rvk( tree);
	tree->generate_sd_tree( tree);
	tree->free( tree);


	tree = nnl_tree_init( 5, 0);
	if( !tree )
	{
		printf( "Unable to allocate the toy tree");
		return 0;
	}

	tree->revoke_node( tree, 0x04000000); // Leaf 00000
	tree->revoke_node( tree, 0x1C000000); // Leaf 00011
	tree->revoke_node( tree, 0x24000000); // Leaf 00100
	tree->revoke_node( tree, 0x4C000000); // Leaf 01001
	tree->revoke_node( tree, 0x74000000); // Leaf 01110
	tree->revoke_node( tree, 0x7C000000); // Leaf 01111
	tree->revoke_node( tree, 0xDC000000); // Leaf 11011
	tree->revoke_node( tree, 0xE4000000); // Leaf 11100
	tree->revoke_node( tree, 0xEC000000); // Leaf 11101
	tree->revoke_node( tree, 0xF4000000); // Leaf 11110
	tree->revoke_node( tree, 0xFC000000); // Leaf 11111

	tree->print_rvk( tree);
	tree->generate_sd_tree( tree);
	tree->free( tree);

	return 1;
}
