#define NNL_CRYPTO_DEF
#include "nnl_crypto.h"

#include <stdio.h>
#include <string.h>

const CK_BYTE d_0[AES_128_SZ] = { 0x76, 0xc8, 0x8b, 0xb0, 0x4c, 0xe2, 0xe6, 0x45, 0x53, 0x27, 0xec, 0x50, 0x89, 0x9f, 0x13, 0x68 };

/*** Well-known constants ***/
/*
static const uint8_t iv_0[AES_128_SZ] = { 0x0B, 0xA0, 0xF8, 0xDD, 0xFE, 0xA6, 0x1F, 0xB3, 0xD8, 0xDF, 0x9F, 0x56, 0x6A, 0x05, 0x0F, 0x78};
static const uint8_t  h_0[AES_128_SZ] = { 0x2D, 0xC2, 0xDF, 0x39, 0x42, 0x03, 0x21, 0xD0, 0xCE, 0xF1, 0xFE, 0x23, 0x74, 0x02, 0x9D, 0x95};
*/
static const uint8_t  s_0[AES_128_SZ] = { 0x7B, 0x10, 0x3C, 0x5D, 0xCB, 0x08, 0xC4, 0xE5, 0x1A, 0x27, 0xB0, 0x17, 0x99, 0x05, 0x3B, 0xD9};

static void die(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);
}

static void dump_openssl_errors(const char *context)
{
	const char *file, *func, *data;
	int line, flags, any = 0;
	unsigned long e;
 
	fprintf( stderr, "[%s] OpenSSL error queue:\n", context ? context : "openssl");
	while ((e = ERR_get_error_all(&file, &line, &func, &data, &flags)) != 0)
	{
		char buf[256];
		ERR_error_string_n( e, buf, sizeof buf);
		fprintf( stderr, "  %s\n", buf);
		fprintf( stderr, "    lib=%s reason=%s at %s:%d %s%s\n",
			ERR_lib_error_string(e),
			ERR_reason_error_string(e),
			file ? file : "?", line,
			func ? func : "?",
			((flags & ERR_TXT_STRING) && data) ? data : ""
		);
		any = 1;
	}
	if (!any)
		fprintf( stderr, "  (empty — no error was queued)\n");
}

EVP_PKEY *load_pubkey(const char *pem)
{
	BIO *bio = BIO_new_mem_buf(pem, -1); /* -1: NUL-terminated string */
	if (!bio) { die("BIO_new_mem_buf failed"); return NULL; }
 
	EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if (!pkey) die("PEM_read_bio_PUBKEY failed");
	return pkey;
}

int verify_sig(EVP_PKEY *pkey, const EVP_MD *md, const unsigned char *msg, size_t msg_len, const unsigned char *sig, size_t sig_len)
{
	int rc = -1;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	if (!ctx) { die("EVP_MD_CTX_new failed"); return -1; }

	if (EVP_DigestVerifyInit(ctx, NULL, md, NULL, pkey) != 1)
	{
		die("EVP_DigestVerifyInit failed");
		dump_openssl_errors("EVP_DigestVerifyInit");
		goto out;
	}
 

	/* One-shot verify; hashes msg internally then checks the signature. */
	rc = EVP_DigestVerify(ctx, sig, sig_len, msg, msg_len);
	/* rc: 1 = valid, 0 = invalid signature, <0 = error */
	if( rc<0 )
	{
		dump_openssl_errors("EVP_DigestVerifyInit");
		dump_openssl_errors("EVP_DigestVerify");
	}

out:
	EVP_MD_CTX_free(ctx);
	return rc;
}

int ecdsa_raw2der(const unsigned char *raw, size_t half, unsigned char *der_out, size_t der_cap)
{
	int ret = -1;
	ECDSA_SIG *sig = ECDSA_SIG_new();
	BIGNUM *r = BN_bin2bn(raw, half, NULL);
	BIGNUM *s = BN_bin2bn(raw + half, half, NULL);

	if (!sig || !r || !s)
		goto out;

	/* On success, ECDSA_SIG_set0 takes ownership of r and s. */
	if (ECDSA_SIG_set0(sig, r, s) != 1)
		goto out;
	r = s = NULL; /* now owned by sig; don't free them ourselves */

	{
		int len = i2d_ECDSA_SIG(sig, NULL); /* query required length */
		if (len < 0 || (size_t)len > der_cap)
			goto out;

		unsigned char *p = der_out;
		len = i2d_ECDSA_SIG(sig, &p);       /* encode into der_out */
		if (len < 0)
			goto out;
		ret = len;
	}

out:
	BN_free(r); /* safe: NULL if already owned by sig */
	BN_free(s);
	ECDSA_SIG_free(sig);
	return ret;
}

/*** EC Signature helpers ***/
static void ec_sig_free( ec_sig_t *self)
{
	if(!self)
		return;

	if(self->raw)    free( self->raw);
	if(self->der)    free( self->der);
	if(self->buffer) free( self->buffer);
	free(self);
}

static int ec_sig_raw2der( struct ecdsa_signature *self)
{
	if(!self) return -1;

	self->der_sz = ecdsa_raw2der( self->raw, self->raw_sz/2, self->der, self->raw_sz+8);
	return 0;
}

static int ec_sig_verify ( ec_sig_t *self, EVP_PKEY *pkey, const EVP_MD *md, size_t msg_len)
{
	if(!self)
		return -1;

	int retcode = verify_sig( pkey, md, self->buffer, msg_len, self->der, self->der_sz);
	switch( retcode )
	{
	case 1:
		printf( "Valid Signature.\n"); 
		break;

	case 0:
		printf( "(!!) INVALID Signature (!!).\n"); 
		break;

	default:
		printf( "(!!) Verification error (!!).\n"); 
		break;
	};

	return retcode;
}

ec_sig_t *ec_sig_init( size_t sig_sz, size_t mkb_sz)
{
	ec_sig_t *instance = (ec_sig_t*) malloc( sizeof(ec_sig_t));
	if(!instance)
		return NULL;

	instance->free    = ec_sig_free;
	instance->raw2der = ec_sig_raw2der;
	instance->verify  = ec_sig_verify;
	instance->raw_sz  = sig_sz;
	instance->raw     = (uint8_t*) malloc( sig_sz);
	instance->der     = (uint8_t*) malloc( sig_sz+8);
	instance->buffer  = (uint8_t*) malloc( mkb_sz);

	if( !instance->raw || !instance->der || !instance->buffer )
	{
		instance->free( instance);
		return NULL;
	}

	return instance;
}

static int aes128_block(const unsigned char *key, const unsigned char *in, unsigned char *out, int encrypt)
{
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	int ok = 0;
	int len = 0;

	if (!ctx)
		return 0;

	/* ECB has no IV; one 16-byte block, padding disabled. */
	if (EVP_CipherInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL, encrypt) != 1)
		goto done;

	EVP_CIPHER_CTX_set_padding(ctx, 0);

	if (EVP_CipherUpdate(ctx, out, &len, in, AES_128_SZ) != 1)
		goto done;

	/* With padding off and a full block, Final produces no extra bytes. */
	if (EVP_CipherFinal_ex(ctx, out + len, &len) != 1)
		goto done;

	ok = 1;
done:
	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

void aes_128e( const key128_t k, const data128_t d, data128_t out)
{
	aes128_block(*k, *d, *out, 1);
}

void aes_128d( const key128_t k, const data128_t d, data128_t out)
{
	aes128_block(*k, *d, *out, 0);
}

void aes_g ( const key128_t x1, const data128_t x2, key128_t result)
{
	aes_128d( x1, x2, result);
	for( int i = 0; i < AES_128_SZ; ++i)
		(*result)[i] ^= (*x2)[i];
}

void aes_g3( const key128_t k, aes_g3_dir dir, key128_t result)
{
	uint8_t seed[AES_128_SZ];
	memcpy( seed, s_0, AES_128_SZ);
	seed[AES_128_SZ-1] += dir;
	aes_g( k, &seed, result);
}

/*
 * Vectors from FIPS-197 Appendix B / C.1 (the canonical AES-128 example):
 *   key        = 000102030405060708090a0b0c0d0e0f
 *   plaintext  = 00112233445566778899aabbccddeeff
 *   ciphertext = 69c4e0d86a7b0430d8cdb78070b4c55a
 */
int self_test(void)
{
	static const unsigned char key[AES_128_SZ] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	static const unsigned char  pt[AES_128_SZ] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
	static const unsigned char  ct[AES_128_SZ] = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};

	unsigned char buf[AES_128_SZ];
	int pass = 1;

	/* cast away the pointer-constness of the typedefs to match signatures */
	key128_t  kp = (key128_t)&key;
	data128_t pp = (data128_t)&pt;
	data128_t cp = (data128_t)&ct;
	data128_t bp = (data128_t)&buf;

	/* ECB encrypt */
	aes_128e(kp, pp, bp);
	if (memcmp(buf, ct, 16) != 0)
		pass = 0;

	/* ECB decrypt */
	aes_128d(kp, cp, bp);
	if (memcmp(buf, pt, 16) != 0)
		pass = 0;

	/* single-block CBC (IV=0) == ECB, so same vector applies *
	aes_128cbce(kp, pp, bp);
	if (memcmp(buf, ct, 16) != 0)
		pass = 0;

	aes_128cbcd(kp, cp, bp);
	if (memcmp(buf, pt, 16) != 0)
		pass = 0;
	*/

	return pass;
}
