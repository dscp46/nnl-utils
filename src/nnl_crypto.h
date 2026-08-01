#ifndef NNL_CRYPTO_H
#define NNL_CRYPTO_H

#include <string.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <p11-kit/pkcs11.h>

#include "crypto_common.h"

#ifndef NNL_CRYPTO_DEF
extern const CK_BYTE d_0[AES_128_SZ];
#endif

static const char la_pubkey[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIHTMIGkBgcqhkjOPQIBMIGYAgEBMCAGByqGSM49AQECFQCdydgTVezOtWC9sJ75\n"
    "6ufEeafX3zAsBBSdydgTVezOtWC9sJ756ufEeafX3AQUQC2tPsHLzRZSSNaOEkXg\n"
    "xNqssdgEKQQuZPwiV4NR5vTMp+uB0KS9xUzOxgkUol3QVEKInbRVx/I8mgcH9cu5\n"
    "AhUAncnYE1XszrVgvcRPVIF7LH9asBcCAQEDKgAEY8Id/7KyeYoTtY1hFmxOSqyK\n"
    "B3ITfsY4gY/Zj6TDC5lnKL9LkX9qJw==\n"
    "-----END PUBLIC KEY-----\n";

#define V1_MKB_SZ	1048576
#define V1_SIG_SZ	40

/* Signature verification facilities */
typedef struct ecdsa_signature ec_sig_t;
struct ecdsa_signature {
	/* Raw r||s signature */
	uint8_t *raw;
	size_t raw_sz;

	/* DER-encoded signature */
	uint8_t *der;
	size_t der_sz;

	/* Subject to be signed */
	uint8_t *buffer;
	size_t buf_sz; // bytes effectively used in buffer
	
	/* Helper function pointers */
	int  (*raw2der)( ec_sig_t* self);
	int  (*verify) ( ec_sig_t *self, EVP_PKEY *pkey, const EVP_MD *md, size_t msg_len);
	void (*free)   ( ec_sig_t* self);
};


typedef unsigned char (*key128_t)[AES_128_SZ];
typedef unsigned char (*data128_t)[AES_128_SZ];
typedef enum aes_g3_direction {
	aes_g3_dir_left = 0,
	aes_g3_dir_processing = 1,
	aes_g3_dir_right = 2,
} aes_g3_dir;

EVP_PKEY *load_pubkey(const char *pem);
int verify_sig(EVP_PKEY *pkey, const EVP_MD *md, const unsigned char *msg, size_t msg_len, const unsigned char *sig, size_t sig_len);
int ecdsa_raw2der(const unsigned char *raw, size_t half, unsigned char *der_out, size_t der_cap);
ec_sig_t *ec_sig_init( size_t sig_sz, size_t mkb_sz);

// AES ECB
void aes_128e( const key128_t k, const data128_t d, data128_t out);
void aes_128d( const key128_t k, const data128_t d, data128_t out);

// One-way function
void aes_g ( const key128_t x1, const data128_t x2, key128_t result);
// Directional one-way function
void aes_g3( const key128_t k, aes_g3_dir dir, key128_t result);

// Self-test the module's primitives. Return 0 if at least one test fails.
int crypto_self_test(void);

#endif	/* NNL_CRYPTO_H */
