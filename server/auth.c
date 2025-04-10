#include <bcrypt.h>
#include <string.h>
#include "auth.h"

char* hash_password(const char *password) {
    char salt[BCRYPT_HASHSIZE];
    char hash[BCRYPT_HASHSIZE];
    bcrypt_gensalt(12, salt);
    bcrypt_hashpw(password, salt, hash);
    return strdup(hash);
}

int verify_password(const char *password, const char *hash) {
    return bcrypt_checkpw(password, hash) == 0;
}