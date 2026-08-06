#ifndef ADDR_ITEM_H
#define ADDR_ITEM_H

#include "nnl_types.h"

typedef struct addr_item addr_item_t;
struct addr_item {
	nnl_addr_t value;
	struct addr_item *next;
};

addr_item_t *addr_item_new( nnl_addr_t addr);

#endif	/* ADDR_ITEM_H */
