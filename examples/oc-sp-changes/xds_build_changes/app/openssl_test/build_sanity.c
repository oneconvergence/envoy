#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

int main(int arc, char *argv[])
{ 
  /* Load the human readable error strings for libcrypto */
  ERR_load_crypto_strings();

  /* Load all digest and cipher algorithms */
  OpenSSL_add_all_algorithms();

  /* Load config file, and other important initialisation */
  OPENSSL_init_crypto(OPENSSL_INIT_NO_ADD_ALL_CIPHERS
                      | OPENSSL_INIT_NO_ADD_ALL_DIGESTS, NULL);

  /* ... Do some crypto stuff here ... */

  /* Clean up */

  /* Removes all digests and ciphers */
  EVP_cleanup();

  /* if you omit the next, a small leak may be left when you make use of the BIO (low level API) for e.g. base64 transformations */
  CRYPTO_cleanup_all_ex_data();

  /* Remove error strings */
  ERR_free_strings();

  return 0;
}
