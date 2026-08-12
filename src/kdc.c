#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "hsm.h"
#include "nnl_tree.h"
#include "procsec.h"
#include "settings.h"

#define USER_PIN_SZ	257

int get_user_pin( char *user_pin, size_t user_pin_len)
{
	struct termios oldt, newt;
	int success = 0;
	tcgetattr( STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ECHO);
	tcsetattr( STDIN_FILENO, TCSANOW, &newt);

	printf( "Enter user PIN: ");
	fflush( stdout);

	if ( fgets( user_pin, user_pin_len, stdin) != NULL) {
		success = 1;
	}

	tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
	printf("\n");
	return success;
}

int main( int argc, char *argv[], char *envp[])
{
	secret_t *user_pin = NULL; 
	settings_t cfg;

	harden_process();
	user_pin = allocate_secret( USER_PIN_SZ);

	if(!parse_args( argc, argv, envp, &cfg))
		return 1;

	if( cfg.run_tests )
	{
		if( !nnl_tree_runtests() )
			return 1;

		return 0;
	}

	hsm_t *hsm = (hsm_t*) malloc( sizeof( hsm_t));
	if(!hsm)
	{
		fprintf( stderr, "malloc failed while allocating hsm_t.\n");
		return -1;
	}

	if(!get_user_pin( user_pin->ptr, user_pin->len))
	{
		fprintf( stderr, "Failed to get user pin.\n");
		free( hsm);
		return 1;
	}

	if(!hsm_init( cfg.p11_module_path, user_pin->ptr, cfg.token_label, cfg.key_label, hsm))
	{
		fprintf( stderr, "Failed to bind HSM.\n");
		explicit_bzero( user_pin, sizeof( user_pin));
		return 2;
	}

	user_pin->free( user_pin);

	hsm_close( hsm);
	free( hsm);
	return 0;
}
