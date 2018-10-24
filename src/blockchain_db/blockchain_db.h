#ifndef MONERO_BLOCKCHAIN_DB_H_
#define MONERO_BLOCKCHAIN_DB_H_

#include "crypto/hash.h"
#include "crypto/crypto.h"
#include "ringct/rctTypes.h"

#pragma pack(push, 1)

/**
 * @brief a struct containing output metadata
 */
typedef struct output_data_t
{
    public_key pubkey;       //!< the output's public key (for spend verification)
    uint64_t           unlock_time;  //!< the output's unlock time (or height)
    uint64_t           height;       //!< the height of the block which created the output
    key           commitment;   //!< the output's amount commitment (for spend verification)
} output_data_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct tx_data_t
{
    uint64_t tx_id;
    uint64_t unlock_time;
    uint64_t block_id;
} tx_data_t;
#pragma pack(pop)

/**
 * @brief a struct containing txpool per transaction metadata
 */
typedef struct txpool_tx_meta_t
{
    hash max_used_block_id;
    hash last_failed_id;
    uint64_t weight;
    uint64_t fee;
    uint64_t max_used_block_height;
    uint64_t last_failed_height;
    uint64_t receive_time;
    uint64_t last_relayed_time;
    // 112 bytes
    uint8_t kept_by_block;
    uint8_t relayed;
    uint8_t do_not_relay;
    uint8_t double_spend_seen: 1;
    uint8_t bf_padding: 7;
    
    uint8_t padding[76]; // till 192 bytes
} txpool_tx_meta_t;



#define DBF_SAFE       1
#define DBF_FAST       2
#define DBF_FASTEST    4
#define DBF_RDONLY     8
#define DBF_SALVAGE 0x10

typedef struct BlockchainDB {
    bool m_open;  //!< Whether or not the BlockchainDB is open/ready for use
    //   mutable epee::critical_section m_synchronization_lock;  //!< A lock, currently for when BlockchainLMDB needs to resize the backing db file
} BlockchainDB;

#endif //MONERO_BLOCKCHAIN_DB_H_
