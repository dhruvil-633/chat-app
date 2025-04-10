#include <sodium.h>
#include <string.h>

void hash_password(const char *password, char *output) {
    if (sodium_init() < 0) {
        exit(1); // Library initialization failed
    }
    
    char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof salt);
    
    if (crypto_pwhash_str(output, password, strlen(password),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        exit(1); // Out of memory
    }
}

int verify_password(const char *password, const char *hash) {
    return crypto_pwhash_str_verify(hash, password, strlen(password)) == 0;
}