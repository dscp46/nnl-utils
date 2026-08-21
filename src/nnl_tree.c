#include "nnl_tree.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utarray.h>
#include <utstack.h>

#include "addr_item.h"

#define FMT_STR_SZ	16

// Function definitions
static void nnl_tree_free( nnl_tree_t *tree);
static void nnl_tree_generate_sd_tree( nnl_tree_t *self);
static void nnl_tree_revoke_node( nnl_tree_t *self, nnl_addr_t addr);

size_t nnl_tree_offset( nnl_addr_t node_addr, const nnl_tree_t *tree)
{
	nnl_addr_t mask, partition_mask;
	uint8_t node_depth;
	nnl_build_mask( node_addr, &mask, &node_depth, NULL);

	// assert( node_depth <= scheme_depth && node_depth >= partition_depth );
	nnl_addr_t partition_num = node_addr & mask;
	for( size_t i=0; i<(NNL_ADDR_BITS-tree->partition_depth); ++i)
		partition_num >>= 1;

	partition_mask = mask;
	for( size_t i=0; i<(size_t)(node_depth-tree->partition_depth); ++i)
		partition_mask <<=1;

	nnl_addr_t partition_index = (node_addr & mask & ~partition_mask);
	for( size_t i=0; i<(NNL_ADDR_BITS-node_depth); ++i)
	{
		mask >>= 1;
		partition_index >>= 1;
		partition_mask >>=1;
	}
	size_t subtree_sz = (size_t)1 << (tree->scheme_depth - tree->partition_depth + 1);
	subtree_sz--;

	size_t nodes_above_us = (nnl_addr_t)1 << (node_depth - tree->partition_depth);
	nodes_above_us--;
	
	return (subtree_sz * (size_t)partition_num) + nodes_above_us + (size_t)partition_index;
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

nnl_state_t nnl_node_state( nnl_addr_t u, const nnl_tree_t *self)
{
	size_t offset = nnl_tree_offset( u, self);
	if( !self )
		return NNL_ST_REVOKED; // FIXME
	// assert( rvk_tree && (offset < (sizeof( rvk_tree ) / sizeof(uint32_t)))) ?

	//printf( "state(%08" PRIx32 "): %s\n", u, (rvk_tree[offset] == 0)? "VALID" : (( rvk_tree[offset] == nnl_nb_leaves( u, scheme_depth) ) ? "REVOKED":"MIXED"));
	if( self->rvk_tree[offset] == 0 )
		return NNL_ST_VALID;

	if( self->rvk_tree[offset] == nnl_nb_leaves( u, self->scheme_depth) )
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

	size_t d = self->scheme_depth;
	size_t p = self->partition_depth;

	size_t nb_lines = d-p+1;
	size_t nb_items_last = 1 << (d-p);
	size_t item_sz = (size_t) ceil( log(nb_items_last) / log(10) );
	size_t line_sz = ( 1 << p ) * nb_items_last * item_sz * 2;
	char format_str[ FMT_STR_SZ];

	char   **lines;
	lines = calloc( sizeof( char*), nb_lines);

	for( size_t i=0; i<nb_lines; ++i )
	{
		lines[i] = malloc( line_sz+1);
		memset( lines[i], ' ', line_sz);
		lines[i][line_sz] = '\0';

		if( !(lines[i]) )
		{
			fprintf( stderr, "Unable to allocate line %zu.\n", i);
			while( i > 0 )
				free( lines[--i]);

			free( lines);
			return;
		}
	}

	size_t idx=0;
	size_t part=0;
	size_t part_offset=0;
	snprintf( format_str,     FMT_STR_SZ, "%%0%zuzu", item_sz);

	do
	{
		for( size_t j=p; j<=d; ++j)
		{
			size_t nb_items_on_line = 1 << (j-p);
			size_t spaces = ( 1 << (d-j+1) )-1;
			size_t heading_sz = ( 1 << (d-j) )-1;
			size_t item_offset = part_offset;

			if( (d-j) > 0 )
				item_offset += (item_sz*heading_sz);

			for( size_t i=0; i<nb_items_on_line; ++i )
			{
				snprintf( lines[j-p] + item_offset, line_sz+1, format_str, self->rvk_tree[idx++]);
				*(lines[j-p] + item_sz + item_offset) = ' ';
				item_offset += item_sz*(spaces+1);
			}
		}
		part_offset += nb_items_last * item_sz * 2;
	} while( ++part < (size_t)(1 << p) );

	for( size_t i=0; i<nb_lines; ++i )
	{
		printf( "%02zu: %s\n", i+p, lines[i]);
		free( lines[i]);
	}

	free( lines);
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
		++(self->rvk_tree[ nnl_tree_offset( addr, self) ]);
		mask <<=1;
		addr &= mask;
		addr |= (nnl_addr_t)1 << shift++;
		--depth;
	}
	while( depth > self->partition_depth );
	
	for( size_t i=0; i<(NNL_ADDR_BITS-self->partition_depth); ++i)
		addr >>=1;

	// Increment subtree root
	++(self->rvk_tree[ addr * nnl_tree_size( self->scheme_depth-self->partition_depth, 0) ]);
}

static void nnl_tree_emit_sd( nnl_addr_t u, nnl_addr_t v, nnl_tree_t *self)
{
	nnl_sd_t uv;
	nnl_encode_uv( u, v, &uv);
	printf( "Emit T[%08"PRIx32"] \\ T[%08"PRIx32"] AKA T%2zu\\T%2zu. UV = %08"PRIx32", U_shift = 0x%02x\n",
		u, v,
		nnl_tree_offset( u, self)+1, nnl_tree_offset( v, self)+1,
		uv.uv, uv.u_shift
	);
	// TODO: check if associated Dk is compromised
	//
}

static void nnl_tree_generate_sd_tree( nnl_tree_t *self)
{
	if( !self )
		return;

	nnl_addr_t cur, i, left_child, right_child;
	nnl_state_t cur_state, left_state, right_state;

	/*
	UT_icd ut_sd_ent_icd;
	UT_array *sd_tree;
	utarray_new( sd_tree, &ut_sd_ent_icd);
	// */
	addr_item_t *stack = NULL, *cur_it;

	// Insert tree root / subroots
	size_t nb_partitions = (size_t)1 << self->partition_depth;
	//for( size_t n=0; n<nb_partitions; ++n)
	for( size_t n=nb_partitions; n>0; n--)
	{
		i = n-1;
		for( size_t j=0; j<(NNL_ADDR_BITS-self->partition_depth); ++j)
			i <<= 1;
		cur = (nnl_root >> self->partition_depth) | i;
		STACK_PUSH( stack, addr_item_new( cur, stack));
	}

	while(!STACK_EMPTY( stack))
	{
		STACK_POP( stack, cur_it);
		cur = cur_it->value;
		free( cur_it);

		if( nnl_depth( cur) >= self->scheme_depth )
			// Do not process leaves
			continue;

		if( self->partition_depth && nnl_depth( cur) == self->partition_depth )
		{
			// TODO: Add an offset entry into the SD Index (to skip unnecessary lookups)
		}

		cur_state = nnl_node_state( cur, self);

		if( cur_state == NNL_ST_VALID )
		{
			if( nnl_depth( cur) == self->partition_depth )
			{
				// Special case: (sub)root is valid
				// Emit T{root} \ Ø := T{root} \ T{l} U T{root} \ T{r}
				nnl_tree_emit_sd( cur, nnl_left( cur), self);
				nnl_tree_emit_sd( cur, nnl_right( cur), self);
				continue;
			}
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
			left_state = nnl_node_state( left_child, self);

			right_child = nnl_right( i);
			right_state = nnl_node_state( right_child, self);

			if( left_state == NNL_ST_REVOKED && right_state == NNL_ST_REVOKED)
				break; // Safeguard, theorically unreachable

			if( left_state == NNL_ST_VALID )
			{
				if( right_state == NNL_ST_REVOKED )
				{
					nnl_tree_emit_sd( cur, right_child, self);
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
					nnl_tree_emit_sd( cur, left_child, self);
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
				nnl_tree_emit_sd( cur, i, self);
				STACK_PUSH( stack, addr_item_new( left_child, stack));
				break;
			}
			else if( left_state == NNL_ST_REVOKED && right_state == NNL_ST_MIXED )
			{
				nnl_tree_emit_sd( cur, i, self);
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

int nnl_emit_device_keys( hsm_t *hsm, nnl_tree_t *tree, nnl_addr_t addr, size_t key_len, nnl_dk_t **dk_list)
{
	if( !hsm || !tree || !addr )
		return 0;

	addr_item_t *subset = NULL, *diff = NULL, *cur_subset, *cur_diff;

	nnl_addr_t u, u_mask, v;

	u_mask = (nnl_addr_t)-1;
	// Done this way because -1 << NNL_ADDR_BITS - tree->partition_depth is UB for a partition-free tree
	for( size_t i=0; i<(NNL_ADDR_BITS - (size_t)tree->partition_depth); ++i)
		u_mask <<= 1;

	u = addr & u_mask;
	u |= ~u_mask & (u_mask >> 1);

	// Special case where the partition head is the root node.
	if( !u )
		u = nnl_root;

	v = addr;

	// Subsets are the nodes in the device path
	// Differences are branches opposite to the device path
	// Walk from the leaf to root, such that we attribute device keys
	do
	{
		STACK_PUSH( diff, addr_item_new( nnl_opposite_branch(v), diff));
		v = nnl_parent( v);
		STACK_PUSH( subset, addr_item_new( v, subset));
	}
	while( v != u );

	while(!STACK_EMPTY( subset))
	{
		STACK_POP( subset, cur_subset);
		u = cur_subset->value;
		free( cur_subset);

		cur_diff = diff;

		// skip all differences that are above our subset
		while( !nnl_is_parent( u, cur_diff->value) )
		{
			cur_diff = cur_diff->next;
			if( cur_diff == NULL )
				goto sd_walk_continue; /* Break from current loop and continue to next subset */
		}

		while( cur_diff != NULL )
		{
			printf( "Emit T[%08" PRIx32 "] \\ T[%08" PRIx32 "] AKA T%2zu\\T%2zu\n", 
				u, cur_diff->value, 
				nnl_tree_offset( u, tree)+1, nnl_tree_offset( cur_diff->value, tree)+1
			);
			cur_diff = cur_diff->next;
		}

	sd_walk_continue:
		;
	}

	// Empty the difference stack
	while(!STACK_EMPTY( diff))
	{
		STACK_POP( diff, cur_diff);
		free( cur_diff);
	}

	return 1;
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
	instance->rvk_tree = calloc( sizeof( size_t), nnl_tree_size( scheme_depth, partition_depth));

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
	nnl_tree_t *tree = nnl_tree_init( 3, 0);
	printf( "Tree size (3, 0): %zu\n", nnl_tree_size( 3, 0));
	printf( "Offset( 101/3): %zu\n", nnl_tree_offset( 0xB0000000, tree));
	tree->free( tree);

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

	if((u = nnl_parent( nnl_parent( 0xD4000000))) != 0xD0000000 )
	{
		fprintf( stderr, "nnl_parent failed to determine the grandparent of 0xD4000000 (got %08" PRIx32 ", expected %08" PRIx32").\n", u, 0xD0000000);
		return 0;
	}

	if((u = nnl_opposite_branch( 0xDC000000)) != 0xD4000000 )
	{
		fprintf( stderr, "nnl_opposite_branch failed: got %08" PRIx32 ", expected %08" PRIx32").\n", u, 0xD4000000);
		return 0;
	}

	printf( "UV: 0x%" PRIx32 ", U_shift: 0x%x\n", uv.uv, uv.u_shift);

	tree = nnl_tree_init( 5, 0);
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

	tree = nnl_tree_init( 5, 2);
	printf( "Tree size (5, 2): %zu\n", nnl_tree_size( 5, 2));
	printf( "Offset( 0110/4): %zu\n", nnl_tree_offset( 0x68000000, tree));
	tree->revoke_node( tree, 0x6C000000); // Leaf 01101
	tree->revoke_node( tree, 0xD4000000); // Leaf 11010
	printf( "Revoked nodes 01101/5 and 11010/5\n");
	tree->print_rvk( tree);
	tree->generate_sd_tree( tree);
	tree->free( tree);

	return 1;
}
