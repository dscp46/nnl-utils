#include "hsm.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nnl_crypto.h"

typedef enum {
	HSM_ERR_MODPATH_UNSET,
	HSM_ERR_PIN_UNSET,
	HSM_ERR_KEYLABEL_UNSET,
	HSM_ERR_AESD_FAILED,
} hsm_error_t;

static const CK_BYTE ZERO_IV[AES_128_SZ] = { 0 };

static void die( const char *what, CK_RV rv)
{
	fprintf( stderr, "%s failed: 0x%08lX\n", what, (unsigned long) rv);
	exit( 1);
}

static int token_label_matches( const CK_TOKEN_INFO *ti, const char *want)
{
	size_t len = sizeof( ti->label);

	while (len > 0 && ti->label[len - 1] == ' ')
		--len;

	return strlen( want) == len && memcmp( ti->label, want, len) == 0;
}

static CK_SLOT_ID select_slot( hsm_t *hsm, const char *want_label)
{
	CK_SLOT_ID_PTR slots = NULL;
	CK_ULONG count = 0;
	CK_SLOT_ID chosen;
	CK_BBOOL found = CK_FALSE;
	CK_RV rv;

	rv = hsm->p11->C_GetSlotList( CK_TRUE, NULL, &count);
	if (rv != CKR_OK || count == 0)
		die("C_GetSlotList(count)", rv);

	slots = calloc( count, sizeof(CK_SLOT_ID));
	if (!slots)
	{ 
		fprintf( stderr, "calloc failed\n");
		exit(1);
	}

	rv = hsm->p11->C_GetSlotList( CK_TRUE, slots, &count);
	if (rv != CKR_OK) {
		free(slots);
		die("C_GetSlotList(list)", rv);
	}

	if (want_label && *want_label)
	{
		for (CK_ULONG i = 0; i < count; i++)
		{
			CK_TOKEN_INFO ti;
			if (hsm->p11->C_GetTokenInfo( slots[i], &ti) != CKR_OK)
				continue;

			if (token_label_matches( &ti, want_label))
			{
				chosen = slots[i];
				found = CK_TRUE;
				break;
			}
		}
		if (!found)
		{
		    fprintf(stderr, "no token with label '%s' found.\n", want_label);
		    free(slots);
		    exit(1);
		}
	} else {
		chosen = slots[0];   /* first token present */
	}

	free(slots);
	return chosen;
}

static CK_OBJECT_HANDLE find_key( hsm_t *hsm, const char *label)
{
	CK_OBJECT_CLASS key_class = CKO_SECRET_KEY;
	CK_KEY_TYPE key_type = CKK_AES;
	CK_ATTRIBUTE tmpl[] = {
		{ CKA_CLASS,    &key_class, sizeof( key_class) },
		{ CKA_KEY_TYPE, &key_type, sizeof( key_type) },
		{ CKA_LABEL,    (void *)label, (CK_ULONG)strlen(label) },
	};

	CK_OBJECT_HANDLE obj = CK_INVALID_HANDLE;
	CK_ULONG found = 0;
	CK_RV rv;

	rv = hsm->p11->C_FindObjectsInit( hsm->session, tmpl, 3);
	if (rv != CKR_OK)
		die("C_FindObjectsInit", rv);

	rv = hsm->p11->C_FindObjects( hsm->session, &obj, 1, &found);
	if (rv != CKR_OK)
		die("C_FindObjects", rv);

	hsm->p11->C_FindObjectsFinal( hsm->session);

	if (!found)
	{
		fprintf(stderr, "Key '%s' not found on token.\n", label);
		exit(1);
	}

	return obj;
}

static CK_MECHANISM_TYPE select_mechanism( hsm_t *hsm, CK_SLOT_ID slot)
{
	CK_MECHANISM_INFO info;

	if (hsm->p11->C_GetMechanismInfo( slot, CKM_AES_ECB, &info) == CKR_OK)
		return CKM_AES_ECB;

	if (hsm->p11->C_GetMechanismInfo( slot, CKM_AES_CBC, &info) == CKR_OK)
		return CKM_AES_CBC;

	fprintf( stderr, "Neither CKM_AES_ECB nor CKM_AES_CBC is available on selected HSM.\n");
	exit(1);
}

int hsm_init( const char *module_path, const char *user_pin, const char *token_label, const char *key_label, hsm_t* hsm)
{
	CK_C_GetFunctionList get_list;
	CK_RV rv;

	if(!hsm)
		return 0;

	if(!module_path)
	{
		hsm->errno = HSM_ERR_MODPATH_UNSET;
		return 0;
	}

	if(!user_pin)
	{
		hsm->errno = HSM_ERR_PIN_UNSET;
		return 0;
	}

	if(!key_label)
	{
		hsm->errno = HSM_ERR_KEYLABEL_UNSET;
		return 0;
	}

	// Open PKCS#11 library
	hsm->module_handle = dlopen( module_path, RTLD_NOW | RTLD_LOCAL);
	if (!hsm->module_handle)
	{
		fprintf( stderr, "dlopen: %s\n", dlerror());
		exit(1);
	}

	// Attempt mapping C_GetFunctionList
	get_list = (CK_C_GetFunctionList) dlsym( hsm->module_handle, "C_GetFunctionList");
	if (!get_list) 
	{
		fprintf( stderr, "dlsym: %s\n", dlerror()); 
		exit(1);
	}

	// Load PKCS#11 function list
	rv = get_list( &hsm->p11);
	if (rv != CKR_OK)
		die( "C_GetFunctionList", rv);

	// Initialize module
	rv = hsm->p11->C_Initialize( NULL);
	if (rv != CKR_OK)
		die( "C_Initialize", rv);

	CK_SLOT_ID slot = select_slot( hsm, token_label);

	// Open session
	rv = hsm->p11->C_OpenSession( slot, CKF_SERIAL_SESSION | CKF_RW_SESSION, NULL, NULL, &hsm->session);
	if (rv != CKR_OK)
		die("C_OpenSession", rv);

	// Log in with user pin
	rv = hsm->p11->C_Login( hsm->session, CKU_USER,	(CK_UTF8CHAR_PTR) user_pin, (CK_ULONG) strlen( user_pin));
	if (rv != CKR_OK && rv != CKR_USER_ALREADY_LOGGED_IN)
		die("C_Login", rv);

	hsm->key = find_key( hsm, key_label);
	hsm->mech_type = select_mechanism( hsm, slot);
	return 1;
}

void hsm_close( hsm_t *hsm)
{
	if( !hsm )
		return;

	hsm->p11->C_Logout( hsm->session);
	hsm->p11->C_CloseSession( hsm->session);
	hsm->p11->C_Finalize( NULL);
	dlclose( hsm->module_handle);
}

static int hsm_aes_d( hsm_t *hsm, const CK_BYTE in[AES_128_SZ], CK_BYTE out[AES_128_SZ])
{
	CK_MECHANISM mech;
	CK_ULONG out_len = AES_128_SZ;
	CK_RV rv;

	if (hsm->mech_type == CKM_AES_CBC) {
		mech.mechanism      = CKM_AES_CBC;
		mech.pParameter     = (CK_VOID_PTR)ZERO_IV;
		mech.ulParameterLen = AES_128_SZ;
	} else {
		mech.mechanism      = CKM_AES_ECB;
		mech.pParameter     = NULL;
		mech.ulParameterLen = 0;
	}

	rv = hsm->p11->C_DecryptInit( hsm->session, &mech, hsm->key);
	if (rv != CKR_OK)
		die("C_DecryptInit", rv);

	rv = hsm->p11->C_Decrypt( hsm->session, (CK_BYTE_PTR) in, AES_128_SZ, out, &out_len);
	if (rv != CKR_OK)
		die("C_Decrypt", rv);

	if (out_len != AES_128_SZ)
	{
		fprintf( stderr, "unexpected output length %lu\n", (unsigned long)out_len);
		exit(1);
	}
	
	return 1;
}

int hsm_derive_node_key( hsm_t *hsm, nnl_addr_t addr, CK_BYTE dk[AES_128_SZ])
{
	if( !hsm || !dk )
		return 0;

	CK_BYTE seed[AES_128_SZ];
	unsigned int carry = 0;
	size_t addr_sz = sizeof( addr);
	uint64_t wide_addr = (uint64_t) addr;
	
	// Load d_0 into the seed
	memcpy( seed, d_0, AES_128_SZ);

	// Add our node address to the seed
	for( int i = AES_128_SZ-1; i>=0; --i)
	{
		size_t byte_pos = (AES_128_SZ - 1) - i;
		unsigned int added_byte = (byte_pos < addr_sz) ? (unsigned int)((wide_addr >> (8*byte_pos)) & 0xFF) : 0u;
		unsigned int acc = (unsigned int) seed[i] + added_byte + carry;
		seed[i] = (CK_BYTE) (acc & 0xFF);
		carry = acc >> 8;
	}

	// AES-D( seed, Kr); Kr being stored into the HSM
	if( !hsm_aes_d( hsm, seed, dk) )
		return 0; // hsm->errno set by hsm_aes_d()

	// AES-G := AES-D( seed, Kr) ^ seed;
	for( int i = 0; i < AES_128_SZ; ++i)
		dk[i] ^= seed[i];

	return 1;
}

