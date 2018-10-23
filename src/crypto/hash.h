#ifndef MONERO_CRYPTO_HASH_H_
#define MONERO_CRYPTO_HASH_H_
#include "hash-ops.h"

typedef struct {
    char data[HASH_SIZE];
} hash;

typedef struct {
    char data[8];
} hash8;



#endif //MONERO_CRYPTO_HASH_H_
