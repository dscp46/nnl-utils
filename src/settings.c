#include "settings.h"

#include <getopt.h>
#include <string.h>
#include <stdio.h>

#define P11_MODULE_ENVVAR_NAME	"P11_MODULE"

static void usage( const char *name)
{
	fprintf( stderr, "Usage: %s <args>\n\n", name);
	fprintf( stderr, "  %-20s %s\n", "-d",              "Run tests");
	fprintf( stderr, "  %-20s %s\n", "-k <label>",      "Scheme root key [Kr] name (required)");
	fprintf( stderr, "  %-20s %s\n", "-p <p11_mod.so>", "Path to PKCS#11 module (required, if " P11_MODULE_ENVVAR_NAME " is not set)");
	fprintf( stderr, "  %-20s %s\n", "-t <token_name>", "HSM Token selector (required)");
}

static char* getenvvar( char *envp[], const char *var_name)
{
	if( !envp || !var_name || !*var_name)
		return NULL;

	char *value, *result = NULL;
	size_t var_name_sz = strlen( var_name);
	while( *envp )
	{
		if((value = strchr( *envp, '=')) != NULL)
		{
			*value = '\0';
			if(!strncmp( *envp, var_name, var_name_sz) )
			{
				result = value + 1;
				break;
			}
		}
		envp++;
	}

	return result;
}

int parse_args( int argc, char *argv[], char *envp[], settings_t *cfg)
{
	if( !argv || !cfg )
		return 0;

	char c;
	cfg->key_label = NULL;
	cfg->p11_module_path = NULL;
	cfg->token_label = NULL;
	cfg->run_tests = 0;

	while((c = getopt( argc, argv, "dk:p:t:")) != -1 )
	{
		switch(c)
		{
		case 'd':
			cfg->run_tests = 1;
			break;

		case 'k':
			cfg->key_label = optarg;
			break;

		case 'p':
			cfg->p11_module_path = optarg;
			break;

		case 't':
			cfg->token_label = optarg;
			break;


		case '?':
			fprintf( stderr, "Option '%c' requires an argument.\n", optopt);
			return 0;
			break;

		default:
			usage( argv[0]);
			return 0;
		}
	}

	if( optind < argc )
	{
		usage( argv[0]);
		return 0;
	}

	if( !cfg->p11_module_path && (cfg->p11_module_path = getenvvar( envp, P11_MODULE_ENVVAR_NAME)) )
		printf( "Using library from P11_LIB: %s\n", cfg->p11_module_path);

	if( !cfg->run_tests && (!cfg->key_label || !cfg->p11_module_path || !cfg->token_label) )
	{
		usage( argv[0]);
		return 0;
	}

	return 1;
}
