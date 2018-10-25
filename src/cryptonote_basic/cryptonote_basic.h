#ifndef MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_
#define MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_

#include "crypto/hash.h"
#include <glib.h>

/* outputs */
typedef struct txout_to_script
{
    public_key *keys;
    size_t keys_size;
    uint8_t *script;
    size_t script_size;
    //    std::vector<crypto::public_key> keys;
    //    std::vector<uint8_t> script;
} txout_to_script;

typedef struct txout_to_scripthash
{
    hash hash;
} txout_to_scripthash;

typedef struct txout_to_key {
    public_key key;
} txout_to_key;

/* inputs */
typedef struct txin_gen{
    size_t height;
} txin_gen;

typedef struct txin_to_script
{
    hash prev;
    size_t prevout;
    uint8_t* sigset;
    size_t sigset_size;
    //    std::vector<uint8_t> sigset;
} txin_to_script;

typedef struct txin_to_scripthash
{
    hash prev;
    size_t prevout;
    txout_to_script script;
    uint8_t* sigset;
    size_t sigset_size;
    //    std::vector<uint8_t> sigset;
} txin_to_scripthash;

typedef struct txin_to_key
{
    uint64_t amount;
    uint64_t key_offsets;
    size_t key_offsets_size;
    //    std::vector<uint64_t> key_offsets;
    key_image k_image;      // double spending protection
} txin_to_key;

typedef struct txin_v {
    txin_gen gen;
    txin_to_script script;
    txin_to_scripthash scripthash;
    txin_to_key key;
} txin_v;

typedef struct txout_target_v {
    txout_to_script script;
    txout_to_scripthash scripthash;
    txout_to_key key;
} txout_target_v;

//typedef std::pair<uint64_t, txout> out_t;
typedef struct tx_out {
    uint64_t amount;
    txout_target_v target;
} tx_out;


typedef struct transaction_prefix {
    // tx information
    size_t   version;
    uint64_t unlock_time;  //number of block (or time), used as a limitation like: spend this tx not early then block/time
    
    txin_v* vin;
    size_t vin_size;
    tx_out* vout;
    size_t vout_size;
    
    //extra
    uint8_t* extra;
    size_t extra_size;
    //    std::vector<txin_v> vin;
    //    std::vector<tx_out> vout;
    //    std::vector<uint8_t> extra;
    
} transaction_prefix;

typedef struct transaction {
    transaction_prefix prefix;
    // hash cash
    hash hash;
    size_t blob_size;
    
    //std::vector<std::vector<crypto::signature> > signatures
    GArray* signatures;
    
    bool hash_valid;
    bool blob_size_valid;
    //    mutable std::atomic<bool> hash_valid;
    //    mutable std::atomic<bool> blob_size_valid;
    
    //    std::vector<std::vector<crypto::signature> > signatures; //count signatures  always the same as inputs count
    //    rct::rctSig rct_signatures;
    
} transaction;



/************************************************************************/
/*                                                                      */
/************************************************************************/
typedef struct block_header
{
    uint8_t major_version;
    uint8_t minor_version;  // now used as a voting mechanism, rather than how this particular block is built
    uint64_t timestamp;
    hash  prev_id;
    uint32_t nonce;
} block_header;


typedef struct block {
    block_header header;
    transaction miner_tx;
    
    //    std::vector<crypto::hash> tx_hashes;
    hash* tx_hashes;
    size_t tx_hashes_size;
    // hash cash
    hash hash;
    
    // hash cash
    bool hash_valid;
} block;


#endif //MONERO_CRYPTONOTE_BASIC_CRYPTONOTE_BASIC_H_
