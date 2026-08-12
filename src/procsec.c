#define _GNU_SOURCE

#include "procsec.h"
 
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef SYS_memfd_secret
#  if defined(__x86_64__) || defined(__aarch64__)
#    define SYS_memfd_secret 447
#  else 
#    error "Define SYS_memfd_secret for your architecture"
#  endif /* __x86_64__ || __aarch64__ */
#endif 	/* !SYS_memfd_secret */

#ifndef DEBUG
static int process_hardened = 0; /* Module-scoped */
#endif	/* !DEBUG */

static int memfd_secret_raw( unsigned int flags)
{
	return (int)syscall( SYS_memfd_secret, flags);
}

void harden_process( void)
{
	// PR_SET_DUMPABLE 0 disables core dumps AND blocks same-UID ptrace attach.
	if( prctl( PR_SET_DUMPABLE, 0L) == -1)
	{
		fprintf( stderr, "prctl(PR_SET_DUMPABLE)\n");
		exit( EXIT_FAILURE);
	}

	// Force coredump size limit to zero, in case dumpable state is changed elsewhere later.
	struct rlimit no_core = { 0, 0 };
	if( setrlimit( RLIMIT_CORE, &no_core) == -1 )
	{
		fprintf( stderr, "setrlimit(RLIMIT_CORE)\n");
		exit( EXIT_FAILURE);
	}

	// Prevent privilege gain via later setuid/setgid execs. Sensible default for a process that touches key material.
	if( prctl( PR_SET_NO_NEW_PRIVS, 1L, 0L, 0L, 0L) == -1 )
	{
		fprintf( stderr, "prctl(PR_SET_NO_NEW_PRIVS)\n");
		exit( EXIT_FAILURE);
	}

#ifndef DEBUG
	process_hardened = 1; /* Module-scoped*/
#endif	/* !DEBUG */
}

static void free_secret( secret_t *self)
{
	if( !self || !self->ptr )
		return;

	explicit_bzero( self->ptr, self->len);

	if( munlock( self->ptr, self->len) == -1 )
	{
		fprintf( stderr, "munlock\n");
		exit( EXIT_FAILURE);
	}

	if( munmap( self->ptr, self->len) == -1 )
	{
		fprintf( stderr, "munmap\n");
		exit( EXIT_FAILURE);
	}
	self->ptr = NULL;
	self->len = 0;
	free( self);
}

void* allocate_secret( size_t size)
{
#ifndef DEBUG
	if( !process_hardened ) /* Module-scoped */
	{
		fprintf( stderr, "Process not hardened, refusing to allocate secure memory.\n");
		exit( EXIT_FAILURE);
	}
#endif	/* !DEBUG */

	int fd;	
	secret_t *instance = (secret_t*) malloc( sizeof( secret_t));
	if( !instance )
		return NULL;

	instance->ptr = NULL;
	instance->len = size;
	instance->free = free_secret;

#if defined(__linux__)
	if((fd = memfd_secret_raw(0)) == -1)
	{
		if (errno == ENOSYS)
			fprintf(stderr, "memfd_secret() unavailable; refusing to run\n");
		else
			fprintf(stderr, "memfd_secret() failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if( ftruncate( fd, (off_t)size) == -1)
	{
		close( fd);
		fprintf( stderr, "ftruncate() failed\n");
		exit( EXIT_FAILURE);
	}

	if((instance->ptr = mmap(NULL, instance->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED )
	{
		close( fd);
		fprintf( stderr, "mmap() failed\n");
		exit( EXIT_FAILURE);
	}
	close( fd);
#else	/* defined(__linux__) */
	if((instance->ptr = mmap(NULL, instance->size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED )
	{
		fprintf( stderr, "mmap() failed\n");
		exit( EXIT_FAILURE);
	}	
#endif	/* defined(__linux__) */
	
	if ( mlock( instance->ptr, instance->len) == -1)
	{
		fprintf( stderr, "mlock() failed\n");
		exit( EXIT_FAILURE);
	}

#ifdef MADV_NOCORE		/* FreeBSD: exclude this region from core dumps */
	if ( madvise( instance->ptr, instance->size, MADV_NOCORE) == -1)
	{
		fprintf( stderr, "madvise( MADV_NOCORE) failed\n");
		exit( EXIT_FAILURE);
	}
#endif	/* MADV_NOCORE */

	return instance;
}
