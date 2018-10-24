#ifndef MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_
#define MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_

#include "crypto/hash.h"

typedef struct block_header
{
    uint8_t major_version;
    uint8_t minor_version;  // now used as a voting mechanism, rather than how this particular block is built
    uint64_t timestamp;
    hash  prev_id;
    uint32_t nonce;
} block_header;


#endif //MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_
