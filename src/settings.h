#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct settings settings_t;
struct settings {
	char *p11_module_path;
	char *token_label;
	char *key_label;
};

int parse_args( int argc, char *argv[], char *envp[], settings_t *cfg);

#endif	/* SETTINGS_H */
