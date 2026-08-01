#ifndef HSM_H
#define HSM_H

#include <p11-kit/pkcs11.h>
#include "crypto_common.h"
#include "nnl_types.h"

typedef struct hsm_ctx hsm_t;
struct hsm_ctx {
	CK_FUNCTION_LIST_PTR p11;
	CK_SESSION_HANDLE session;
	CK_OBJECT_HANDLE key;
	CK_MECHANISM_TYPE mech_type;
	void *module_handle;
	int err_no;
};

int hsm_init( const char *module_path, const char *user_pin, const char *token_label, const char *key_label, hsm_t* hsm);
void hsm_close( hsm_t *hsm);

// Derive a node key from the scheme root key. Dk must be zeroized after use.
int hsm_derive_node_key( hsm_t *hsm, nnl_addr_t addr, CK_BYTE dk[AES_128_SZ]);

#endif	/* HSM_H */
