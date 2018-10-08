#include <stdint.h>
#include <stdbool.h>
#include <lmdb.h>
#include "glib.h"
#include "blockchain_db/blockchain_db.h"
#include "cryptonote_config.h"

typedef struct mdb_txn_cursors
{
  MDB_cursor *m_txc_blocks;
  MDB_cursor *m_txc_block_heights;
  MDB_cursor *m_txc_block_info;

  MDB_cursor *m_txc_output_txs;
  MDB_cursor *m_txc_output_amounts;

  MDB_cursor *m_txc_txs;
  MDB_cursor *m_txc_txs_pruned;
  MDB_cursor *m_txc_txs_prunable;
  MDB_cursor *m_txc_txs_prunable_hash;
  MDB_cursor *m_txc_tx_indices;
  MDB_cursor *m_txc_tx_outputs;

  MDB_cursor *m_txc_spent_keys;

  MDB_cursor *m_txc_txpool_meta;
  MDB_cursor *m_txc_txpool_blob;

  MDB_cursor *m_txc_hf_versions;
} mdb_txn_cursors;

#define m_cur_blocks	m_cursors->m_txc_blocks
#define m_cur_block_heights	m_cursors->m_txc_block_heights
#define m_cur_block_info	m_cursors->m_txc_block_info
#define m_cur_output_txs	m_cursors->m_txc_output_txs
#define m_cur_output_amounts	m_cursors->m_txc_output_amounts
#define m_cur_txs	m_cursors->m_txc_txs
#define m_cur_txs_pruned	m_cursors->m_txc_txs_pruned
#define m_cur_txs_prunable	m_cursors->m_txc_txs_prunable
#define m_cur_txs_prunable_hash	m_cursors->m_txc_txs_prunable_hash
#define m_cur_tx_indices	m_cursors->m_txc_tx_indices
#define m_cur_tx_outputs	m_cursors->m_txc_tx_outputs
#define m_cur_spent_keys	m_cursors->m_txc_spent_keys
#define m_cur_txpool_meta	m_cursors->m_txc_txpool_meta
#define m_cur_txpool_blob	m_cursors->m_txc_txpool_blob
#define m_cur_hf_versions	m_cursors->m_txc_hf_versions

typedef struct mdb_rflags
{
  bool m_rf_txn;
  bool m_rf_blocks;
  bool m_rf_block_heights;
  bool m_rf_block_info;
  bool m_rf_output_txs;
  bool m_rf_output_amounts;
  bool m_rf_txs;
  bool m_rf_txs_pruned;
  bool m_rf_txs_prunable;
  bool m_rf_txs_prunable_hash;
  bool m_rf_tx_indices;
  bool m_rf_tx_outputs;
  bool m_rf_spent_keys;
  bool m_rf_txpool_meta;
  bool m_rf_txpool_blob;
  bool m_rf_hf_versions;
} mdb_rflags;

typedef struct mdb_threadinfo
{
  MDB_txn *m_ti_rtxn;	// per-thread read txn
  mdb_txn_cursors m_ti_rcursors;	// per-thread read cursors
  mdb_rflags m_ti_rflags;	// per-thread read state

  // ~mdb_threadinfo();
} mdb_threadinfo;


typedef struct mdb_txn_safe
{
  // mdb_txn_safe(const bool check=true);
  // ~mdb_txn_safe();

  // void commit(char* message = "");


  // void abort();
  // void uncheck();

  // operator MDB_txn*()
  // {
  //   return m_txn;
  // }

  // operator MDB_txn**()
  // {
  //   return &m_txn;
  // }

  // uint64_t num_active_tx() const;

  // static void prevent_new_txns();
  // static void wait_no_active_txns();
  // static void allow_new_txns();

  mdb_threadinfo* m_tinfo;
  MDB_txn* m_txn;
  bool m_batch_txn;
  bool m_check;
} mdb_txn_safe;

void mdb_txn_safe_commit(mdb_txn_safe* txn, const char* message);
// This should only be needed for batch transaction which must be ensured to
// be aborted before mdb_env_close, not after. So we can't rely on
// BlockchainLMDB destructor to call mdb_txn_safe destructor, as that's too late
// to properly abort, since mdb_env_close would have been called earlier.
void mdb_txn_safe_abort(mdb_txn_safe* txn);
void mdb_txn_safe_uncheck(mdb_txn_safe* txn);
uint64_t mdb_txn_safe_num_active_tx();

static void mdb_txn_safe_prevent_new_txns();
static void mdb_txn_safe_wait_no_active_txns();
static void mdb_txn_safe_allow_new_txns();

static sig_atomic_t mdb_txn_safe_num_active_txns;
// static std::atomic<uint64_t> num_active_txns;

// could use a mutex here, but this should be sufficient.
static sig_atomic_t mdb_txn_safe_creation_gate;
// static std::atomic_flag creation_gate;

typedef struct BlockchainLMDB {
  BlockchainDB* db;
  MDB_env* m_env;

  MDB_dbi m_blocks;
  MDB_dbi m_block_heights;
  MDB_dbi m_block_info;

  MDB_dbi m_txs;
  MDB_dbi m_txs_pruned;
  MDB_dbi m_txs_prunable;
  MDB_dbi m_txs_prunable_hash;
  MDB_dbi m_tx_indices;
  MDB_dbi m_tx_outputs;

  MDB_dbi m_output_txs;
  MDB_dbi m_output_amounts;

  MDB_dbi m_spent_keys;

  MDB_dbi m_txpool_meta;
  MDB_dbi m_txpool_blob;

  MDB_dbi m_hf_starting_heights;
  MDB_dbi m_hf_versions;

  MDB_dbi m_properties;
  //mutable uint64_t m_cum_size;
  uint64_t m_cum_size;	// used in batch size estimation
  //mutable unsigned int m_cum_count;
  unsigned int m_cum_count;
  char* m_folder;
  mdb_txn_safe* m_write_txn; // may point to either a short-lived txn or a batch txn
  mdb_txn_safe* m_write_batch_txn; // persist batch txn outside of BlockchainLMDB
  // boost::thread::id m_writer;
  uint64_t m_writer;

  bool m_batch_transactions; // support for batch transactions
  bool m_batch_active; // whether batch transaction is in progress

  mdb_txn_cursors m_wcursors;
  // mutable boost::thread_specific_ptr<mdb_threadinfo> m_tinfo;
  //TODO thread safe
  mdb_threadinfo* m_tinfo;

} BlockchainLMDB;


void lmdb_open(BlockchainLMDB *lmdb,const char* filename, const int db_flags);


#if defined(ENABLE_AUTO_RESIZE)
static uint64_t DEFAULT_MAPSIZE = 1LL << 30;
#else
static uint64_t DEFAULT_MAPSIZE = 1LL << 33;
#endif
