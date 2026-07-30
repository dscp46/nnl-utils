#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "nnl_tree.h"

int main( int argc, char *argv[], char *envp[])
{
	printf( "Tree size (3): %" PRIu32 "\n", nnl_tree_size( 3));
	printf( "Offset( 101/3): %" PRIu32 "\n", nnl_tree_offset(0xB0000000));

	nnl_addr_t u = 0xB0000000;
	nnl_addr_t v = 0xA2D80000;
	nnl_addr_t u_prime, v_prime;
	nnl_sd_t uv;

	if(!nnl_encode_uv( u, v, &uv))
	{
		fprintf( stderr, "Failed to encode UV for U: 0x%" PRIx32 ", V: 0x%" PRIx32 "\n", u, v);
		return 1;
	}

	if(!nnl_decode_uv( &uv, &u_prime, &v_prime))
	{
		fprintf( stderr, "Failed to decode UV for UV: 0x%" PRIx32 ", U_shift: 0x%x\n", uv.uv, uv.u_shift);
		return 1;
	}

	if( u != u_prime || v != v_prime )
	{
		fprintf( stderr, "U (0x%" PRIx32") != U' (0x%"PRIx32")\n", u, u_prime);
		return 1;
	}

	if( v != v_prime )
	{
		fprintf( stderr, "V (0x%" PRIx32") != V' (0x%"PRIx32")\n", v, v_prime);
		return 1;
	}

	printf( "UV: 0x%" PRIx32 ", U_shift: 0x%x\n", uv.uv, uv.u_shift);

	return 0;
}
