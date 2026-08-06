#include "settings.h"

#include <getopt.h>
#include <stdio.h>

static void usage( const char *name)
{
	fprintf( stderr, "Usage: %s <args>\n\n", name);
	fprintf( stderr, "  %-20s %s\n", "-d",              "Run tests");
	fprintf( stderr, "  %-20s %s\n", "-k <label>",      "Scheme root key [Kr] name (required)");
	fprintf( stderr, "  %-20s %s\n", "-p <p11_mod.so>", "Path to PKCS#11 module (required)");
	fprintf( stderr, "  %-20s %s\n", "-t <token_name>", "HSM Token selector (required)");
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

	if( !cfg->run_tests && (!cfg->key_label || !cfg->p11_module_path || !cfg->token_label) )
	{
		usage( argv[0]);
		return 0;
	}

	return 1;
}
