#ifndef PROCSEC_H
#define PROCSEC_H

#include <stddef.h>

typedef struct secret secret_t;
struct secret {
	void* ptr;
	size_t len;
	void (*free)( secret_t *self);
};

void  harden_process( void);
void* allocate_secret( size_t size);

#endif	/* PROCSEC_H */
