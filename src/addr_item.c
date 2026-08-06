#include "addr_item.h"

#include <stdlib.h>

addr_item_t *addr_item_new( nnl_addr_t addr)
{
	addr_item_t *instance = (addr_item_t*) malloc( sizeof( addr_item_t));
	
	if( instance )
	       instance->value = addr;

	return instance;
}
