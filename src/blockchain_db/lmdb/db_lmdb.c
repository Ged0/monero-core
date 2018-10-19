#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "common/file_util.h"
#include "db_lmdb.h"

// Increase when the DB structure changes
#define VERSION 3

MDB_val* mdb_val_from_char_array(char* val) {
    MDB_val *mdb_val = malloc(sizeof(MDB_val));
    mdb_val->mv_size = sizeof(strlen(val) + 1);
    mdb_val->mv_data = val;
    return mdb_val;
}

MDB_val* mdb_val_from_uint32_t(uint32_t val) {
    MDB_val *mdb_val = malloc(sizeof(MDB_val));
    mdb_val->mv_size = sizeof(uint32_t);
    mdb_val->mv_data = &val;
    return mdb_val;
}

int compare_uint64(const MDB_val *a, const MDB_val *b) {
    const uint64_t va = *(const uint64_t *)a->mv_data;
    const uint64_t vb = *(const uint64_t *)b->mv_data;
    return (va < vb) ? -1 : va > vb;
}

int compare_hash32(const MDB_val *a, const MDB_val *b) {
    uint32_t *va = (uint32_t*) a->mv_data;
    uint32_t *vb = (uint32_t*) b->mv_data;
    for (int n = 7; n >= 0; n--) {
        if (va[n] == vb[n])
            continue;
        return va[n] < vb[n] ? -1 : 1;
    }
    
    return 0;
}

int compare_string(const MDB_val *a, const MDB_val *b) {
    const char *va = (const char*) a->mv_data;
    const char *vb = (const char*) b->mv_data;
    return strcmp(va, vb);
}

void mdb_txn_safe_commit(mdb_txn_safe* txn, const char* message) {
    if (message == NULL || strlen(message) == 0) {
        message = "Failed to commit a transaction to the db";
    }
    int result = mdb_txn_commit(txn->m_txn);
    txn->m_txn = NULL;
    if (result) {
        g_error("%s: %d", message, result);
    }
}
void mdb_txn_safe_abort(mdb_txn_safe* txn) {
    g_debug("mdb_txn_safe: abort()");
    if (txn != NULL && txn->m_txn != NULL) {
        mdb_txn_abort(txn->m_txn);
        txn->m_txn = NULL;
    } else {
        g_warning("WARNING: mdb_txn_safe: abort() called, but m_txn is NULL");
    }
}
static void mdb_txn_safe_prevent_new_txns() {
    //TODO
}
static void mdb_txn_safe_wait_no_active_txns() {
    //TODO
}
static void mdb_txn_safe_allow_new_txns() {
    //TODO
}

/* DB schema:
 *
 * Table            Key          Data
 * -----            ---          ----
 * blocks           block ID     block blob
 * block_heights    block hash   block height
 * block_info       block ID     {block metadata}
 *
 * txs_pruned       txn ID       pruned txn blob
 * txs_prunable     txn ID       prunable txn blob
 * txs_prunable_hash txn ID      prunable txn hash
 * tx_indices       txn hash     {txn ID, metadata}
 * tx_outputs       txn ID       [txn amount output indices]
 *
 * output_txs       output ID    {txn hash, local index}
 * output_amounts   amount       [{amount output index, metadata}...]
 *
 * spent_keys       input hash   -
 *
 * txpool_meta      txn hash     txn metadata
 * txpool_blob      txn hash     txn blob
 *
 * Note: where the data items are of uniform size, DUPFIXED tables have
 * been used to save space. In most of these cases, a dummy "zerokval"
 * key is used when accessing the table; the Key listed above will be
 * attached as a prefix on the Data to serve as the DUPSORT key.
 * (DUPFIXED saves 8 bytes per record.)
 *
 * The output_amounts table doesn't use a dummy key, but uses DUPSORT.
 */
const char* const LMDB_BLOCKS = "blocks";
const char* const LMDB_BLOCK_HEIGHTS = "block_heights";
const char* const LMDB_BLOCK_INFO = "block_info";

const char* const LMDB_TXS = "txs";
const char* const LMDB_TXS_PRUNED = "txs_pruned";
const char* const LMDB_TXS_PRUNABLE = "txs_prunable";
const char* const LMDB_TXS_PRUNABLE_HASH = "txs_prunable_hash";
const char* const LMDB_TX_INDICES = "tx_indices";
const char* const LMDB_TX_OUTPUTS = "tx_outputs";

const char* const LMDB_OUTPUT_TXS = "output_txs";
const char* const LMDB_OUTPUT_AMOUNTS = "output_amounts";
const char* const LMDB_SPENT_KEYS = "spent_keys";

const char* const LMDB_TXPOOL_META = "txpool_meta";
const char* const LMDB_TXPOOL_BLOB = "txpool_blob";

const char* const LMDB_HF_STARTING_HEIGHTS = "hf_starting_heights";
const char* const LMDB_HF_VERSIONS = "hf_versions";

const char* const LMDB_PROPERTIES = "properties";

const char zerokey[8] = {0};
const MDB_val zerokval = { sizeof(zerokey), (void *)zerokey };

const char* lmdb_error(const char* error_string, int mdb_res) {
    char* mdb_error_string = mdb_strerror(mdb_res);
    char* full_string = malloc(sizeof(char) *(strlen(error_string) + strlen(mdb_error_string) + 4));
    strcpy(full_string, error_string);
    strcpy(full_string, " : ");
    strcat(full_string, mdb_error_string);
    return full_string;
}

static inline void lmdb_db_open(MDB_txn* txn, const char* name, int flags, MDB_dbi *dbi, const char* error_string) {
    int res = mdb_dbi_open(txn, name, flags, dbi);
    if (res) {
        g_error("%s - you may want to start with --db-salvage", lmdb_error(error_string, res));
    }
}

int lmdb_open(BlockchainLMDB *lmdb, const char* filename, const int db_flags) {
    int result;
    int mdb_flags = MDB_NORDAHEAD;

    if (lmdb != NULL && lmdb->db->m_open) {
        /* code */
        g_info("Attempted to open db, but it's already open");
        return -1;
    }
    
    struct stat sb;
    if (stat(filename, &sb) != 0) {
        if ((result = mkdir(filename, 0777))) {
            g_info("Create file failed, filename: %s, result: %d", filename, result);
            return -2;
        }
    }
    if (!S_ISDIR(sb.st_mode)) {
        g_info("LMDB needs a directory path, but a file was passed");
        return -3;
    }
    
    int index = (int)(strrchr(filename, '/') - filename);
    char oldFiles[index+1];
    strncpy(oldFiles, filename, index+1);
    
    char block_data_file_path[strlen(oldFiles) + strlen(CRYPTONOTE_BLOCKCHAINDATA_FILENAME)];
    strcpy(block_data_file_path, oldFiles);
    strcat(block_data_file_path, CRYPTONOTE_BLOCKCHAINDATA_FILENAME);
    char block_lock_file_path[strlen(oldFiles) + strlen(CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME)];
    strcpy(block_lock_file_path, oldFiles);
    strcat(block_lock_file_path, CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME);
    if (is_file_exists(block_data_file_path) || is_file_exists(block_lock_file_path)) {
        g_info("Found existing LMDB files in %s", oldFiles);
        g_info("Move %s and/or %s to %s, or delete them, and then restart", block_data_file_path, block_lock_file_path, oldFiles);
        return -4;
    }
    
    //TODO check is hdd.
//    boost::optional<bool> is_hdd_result = tools::is_hdd(filename.c_str());
//    if (is_hdd_result)
//    {
//        if (is_hdd_result.value())
//            MCLOG_RED(el::Level::Warning, "global", "The blockchain is on a rotating drive: this will be very slow, use a SSD if possible");
//    }

    lmdb->m_folder = malloc(strlen(filename) + 1);
    strncpy(lmdb->m_folder, filename, strlen(filename));
    
    if((result = mdb_env_create(&(lmdb->m_env)))) {
        g_info("Failed to create lmdb environment: %d", result);
        return -5;
    }
    
    if ((result = mdb_env_set_maxdbs(lmdb->m_env, 20))) {
        g_info("Failed to set max number of dbs: %d", result);
        return -6;
    }
    
    //TODO need to calculate from core of CPU
    int threads = 20;
    if (threads > 110
        && (result = mdb_env_set_maxreaders(lmdb->m_env, threads + 16))) {
        g_info("Failed to set max number of readers: %d", result);
        return -7;
    }
    
    size_t mapsize = DEFAULT_MAPSIZE;
    
    if (db_flags & DBF_FAST)
        mdb_flags |= MDB_NOSYNC;
    if (db_flags & DBF_FASTEST)
        mdb_flags |= MDB_NOSYNC | MDB_WRITEMAP | MDB_MAPASYNC;
    if (db_flags & DBF_RDONLY)
        mdb_flags = MDB_RDONLY;
    if (db_flags & DBF_SALVAGE)
        mdb_flags |= MDB_PREVSNAPSHOT;
    
    if ((result = mdb_env_open(lmdb->m_env, filename, mdb_flags, 0644))) {
        g_info("Failed to open lmdb environment: %d", result);
        return -8;
    }
    
    MDB_envinfo mei;
    mdb_env_info(lmdb->m_env, &mei);
    uint64_t cur_mapsize = (double)mei.me_mapsize;
    
    if (cur_mapsize < mapsize) {
        if ((result = mdb_env_set_mapsize(lmdb->m_env, mapsize))) {
            g_info("Failed to set max memory map size: %d", result);
            return -9;
        }
        mdb_env_info(lmdb->m_env, &mei);
        cur_mapsize = (double)mei.me_mapsize;
        g_info("LMDB memory map size: %llu", cur_mapsize);
    }
    
    if (lmdb_need_resize(lmdb, 0)) {
        lmdb_do_resize(lmdb, 0);
    }
    
    int txn_flags = 0;
    if (mdb_flags & MDB_RDONLY) {
        txn_flags |= MDB_RDONLY;
    }
    
    // get a read/write MDB_txn, depending on mdb_flags
    mdb_txn_safe txn_safe;
    int mdb_res = mdb_txn_begin(lmdb->m_env, NULL, txn_flags, &txn_safe.m_txn);
    if (mdb_res) {
        g_info("Failed to create a transaction for the db: %d", mdb_res);
        return -10;
    }
    MDB_txn *txn = txn_safe.m_txn;
    
    // open necessary databases, and set properties as needed
    // uses macros to avoid having to change things too many places
    lmdb_db_open(txn, LMDB_BLOCKS, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_blocks, "Failed to open db handle for m_blocks");
    
    lmdb_db_open(txn, LMDB_BLOCK_INFO, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &lmdb->m_block_info, "Failed to open db handle for m_block_info");
    lmdb_db_open(txn, LMDB_BLOCK_HEIGHTS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &lmdb->m_block_heights, "Failed to open db handle for m_block_heights");
    
    lmdb_db_open(txn, LMDB_TXS, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_txs, "Failed to open db handle for m_txs");
    lmdb_db_open(txn, LMDB_TXS_PRUNED, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_txs_pruned, "Failed to open db handle for m_txs_pruned");
    lmdb_db_open(txn, LMDB_TXS_PRUNABLE, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_txs_prunable, "Failed to open db handle for m_txs_prunable");
    lmdb_db_open(txn, LMDB_TXS_PRUNABLE_HASH, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_txs_prunable_hash, "Failed to open db handle for m_txs_prunable_hash");
    lmdb_db_open(txn, LMDB_TX_INDICES, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &lmdb->m_tx_indices, "Failed to open db handle for m_tx_indices");
    lmdb_db_open(txn, LMDB_TX_OUTPUTS, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_tx_outputs, "Failed to open db handle for m_tx_outputs");
    
    lmdb_db_open(txn, LMDB_OUTPUT_TXS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &lmdb->m_output_txs, "Failed to open db handle for m_output_txs");
    lmdb_db_open(txn, LMDB_OUTPUT_AMOUNTS, MDB_INTEGERKEY | MDB_DUPSORT | MDB_DUPFIXED | MDB_CREATE, &lmdb->m_output_amounts, "Failed to open db handle for m_output_amounts");
    
    lmdb_db_open(txn, LMDB_SPENT_KEYS, MDB_INTEGERKEY | MDB_CREATE | MDB_DUPSORT | MDB_DUPFIXED, &lmdb->m_spent_keys, "Failed to open db handle for m_spent_keys");
    
    lmdb_db_open(txn, LMDB_TXPOOL_META, MDB_CREATE, &lmdb->m_txpool_meta, "Failed to open db handle for m_txpool_meta");
    lmdb_db_open(txn, LMDB_TXPOOL_BLOB, MDB_CREATE, &lmdb->m_txpool_blob, "Failed to open db handle for m_txpool_blob");
    
    // this subdb is dropped on sight, so it may not be present when we open the DB.
    // Since we use MDB_CREATE, we'll get an exception if we open read-only and it does not exist.
    // So we don't open for read-only, and also not drop below. It is not used elsewhere.
    if (!(mdb_flags & MDB_RDONLY)) {
        lmdb_db_open(txn, LMDB_HF_STARTING_HEIGHTS, MDB_CREATE, &lmdb->m_hf_starting_heights, "Failed to open db handle for m_hf_starting_heights");
    }
    
    lmdb_db_open(txn, LMDB_HF_VERSIONS, MDB_INTEGERKEY | MDB_CREATE, &lmdb->m_hf_versions, "Failed to open db handle for m_hf_versions");
    
    lmdb_db_open(txn, LMDB_PROPERTIES, MDB_CREATE, &lmdb->m_properties, "Failed to open db handle for m_properties");
    
    mdb_set_dupsort(txn, lmdb->m_spent_keys, compare_hash32);
    mdb_set_dupsort(txn, lmdb->m_block_heights, compare_hash32);
    mdb_set_dupsort(txn, lmdb->m_tx_indices, compare_hash32);
    mdb_set_dupsort(txn, lmdb->m_output_amounts, compare_uint64);
    mdb_set_dupsort(txn, lmdb->m_output_txs, compare_uint64);
    mdb_set_dupsort(txn, lmdb->m_block_info, compare_uint64);
    
    mdb_set_compare(txn, lmdb->m_txpool_meta, compare_hash32);
    mdb_set_compare(txn, lmdb->m_txpool_blob, compare_hash32);
    mdb_set_compare(txn, lmdb->m_properties, compare_string);
    
    if (!(mdb_flags & MDB_RDONLY)) {
        result = mdb_drop(txn, lmdb->m_hf_starting_heights, 1);
        if (result && result != MDB_NOTFOUND) {
            g_info("Failed to drop m_hf_starting_heights: %d", result);
            return -11;
        }
    }
    
    // get and keep current height
    MDB_stat db_stats;
    if ((result = mdb_stat(txn, lmdb->m_blocks, &db_stats))) {
        g_info("%s", lmdb_error("Failed to query m_blocks: ", result));
        return -12;
    }
    g_info("Setting m_height to: %zu", db_stats.ms_entries);
    uint64_t m_height = db_stats.ms_entries;
    
    bool compatible = true;
    
    MDB_val* k = mdb_val_from_char_array("version");
    MDB_val v;
    int get_result = mdb_get(txn, lmdb->m_properties, k, &v);
    if (get_result == MDB_SUCCESS) {
        const uint32_t db_version = *(const uint32_t*)v.mv_data;
        if (db_version > VERSION) {
            g_warning("Existing lmdb database was made by a later version (%d). We don't know how it will change yet.", db_version);
            compatible = false;
        }
#if VERSION > 0
        else if (db_version < VERSION) {
            // Note that there was a schema change within version 0 as well.
            // See commit e5d2680094ee15889934fe28901e4e133cda56f2 2015/07/10
            // We don't handle the old format previous to that commit.
            mdb_txn_safe_commit(&txn_safe, NULL);
            lmdb->db->m_open = true;
            lmdb_migrate(lmdb, 0);
            return 0;
        }
#endif
    } else {
        // if not found, and the DB is non-empty, this is probably
        // an "old" version 0, which we don't handle. If the DB is
        // empty it's fine.
        if (VERSION > 0 && m_height > 0)
            compatible = false;
    }
    
    if (!compatible) {
        mdb_txn_safe_abort(&txn_safe);
        mdb_env_close(lmdb->m_env);
        lmdb->db->m_open = false;
        g_info("Existing lmdb database is incompatible with this version.\nPlease delete the existing database and resync.");
        return -13;
    }
    
    if (!(mdb_flags & MDB_RDONLY)) {
        // only write version on an empty DB
        if (m_height == 0) {
            MDB_val* k = mdb_val_from_char_array("version");
            MDB_val* v = mdb_val_from_uint32_t(VERSION);
            int put_result = mdb_put(txn, lmdb->m_properties, k, v, 0);
            if (put_result != MDB_SUCCESS) {
                mdb_txn_safe_abort(&txn_safe);
                mdb_env_close(lmdb->m_env);
                lmdb->db->m_open = false;
                g_info("Failed to write version to database.");
                return -14;
            }
        }
    }
    
    // commit the transaction
    mdb_txn_safe_commit(&txn_safe, NULL);
    
    lmdb->db->m_open = true;
    // from here, init should be finished
    return 0;
}

bool lmdb_is_read_only(BlockchainLMDB *lmdb) {
    unsigned int flags;
    int result = mdb_env_get_flags(lmdb->m_env, &flags);
    if (result) {
        g_error("Error getting database environment info:  %d", result);
    }
    if (flags & MDB_RDONLY) {
        return true;
    } else {
        return false;
    }
}

int lmdb_close(BlockchainLMDB *lmdb) {
    if (lmdb->m_batch_active) {
        g_warning("close() first calling batch_abort() due to active batch transaction");
        lmdb_batch_abort(lmdb);
    }
    lmdb_sync(lmdb);
    //TODO
//    m_tinfo.reset();
    mdb_env_close(lmdb->m_env);
    lmdb->db->m_open = false;
    return 0;
}

int lmdb_sync(BlockchainLMDB* lmdb) {
    g_debug("lmdb_sync");
    if (!lmdb->db->m_open) {
        g_info("DB operation attempted on a not-open DB instance");
        return -1;
    }
    if (lmdb_is_read_only(lmdb)) {
        return 0;
    }
    int result = mdb_env_sync(lmdb->m_env, true);
    if (result) {
        g_info("Failed to sync database: %d", result);
        return result;
    }
    return 0;
}

int lmdb_safe_sync_mode(BlockchainLMDB* lmdb, const bool onoff) {
    g_info("switching safe mode %s", (onoff ? "on" : "off"));
    mdb_env_set_flags(lmdb->m_env, MDB_NOSYNC|MDB_MAPASYNC, !onoff);
    return 0;
}

int lmdb_reset(BlockchainLMDB* lmdb) {
    g_info("lmdb_reset");
    if (!lmdb->db->m_open) {
        return -1;
    }
    mdb_txn_safe txn;
    //TODO
//    int result = lmdb_txn_begin(lmdb->m_env, NULL, 0, &txn);
    return 0;
}

int lmdb_batch_abort(BlockchainLMDB* lmdb) {
    g_debug("lmdb_batch_abort");
    if (!lmdb->m_batch_transactions) {
        g_info("batch transactions not enabled");
        return -1;
    }
    if (!lmdb->m_batch_active) {
        g_info("batch transaction not in progress");
        return -2;
    }
    if (!lmdb->m_write_batch_txn) {
        g_info("batch transaction not in progress");
        return -3;
    }
    //TODO
//    if (m_writer != boost::this_thread::get_id())
//        throw1(DB_ERROR("batch transaction owned by other thread"));
    if (!lmdb->db->m_open) {
        g_info("DB operation attempted on a not-open DB instance");
        return -4;
    }
    // for destruction of batch transaction
    lmdb->m_write_txn = NULL;
    // explicitly call in case mdb_env_close() (BlockchainLMDB::close()) called before BlockchainLMDB destructor called.
    mdb_txn_safe_abort(lmdb->m_write_txn);
    free(lmdb->m_write_txn);
    lmdb->m_write_batch_txn = NULL;
    lmdb->m_batch_active = false;
    memset(&(lmdb->m_wcursors), 0, sizeof(lmdb->m_wcursors));
    g_info("batch transaction: aborted");
    return 0;
}

bool lmdb_need_resize(BlockchainLMDB *lmdb, uint64_t threshold_size) {
#if defined(ENABLE_AUTO_RESIZE)
    MDB_envinfo mei;
    
    mdb_env_info(lmdb->m_env, &mei);
    
    MDB_stat mst;
    
    mdb_env_stat(lmdb->m_env, &mst);
    
    // size_used doesn't include data yet to be committed, which can be
    // significant size during batch transactions. For that, we estimate the size
    // needed at the beginning of the batch transaction and pass in the
    // additional size needed.
    uint64_t size_used = mst.ms_psize * mei.me_last_pgno;
    
    g_info("DB map size:     %zu", mei.me_mapsize);
    g_info("Space used:      %llu", size_used);
    g_info("Space remaining: %llu", mei.me_mapsize - size_used);
    g_info("Size threshold:  %llu", threshold_size);
    float resize_percent = RESIZE_PERCENT;
    g_info("Percent used: %f  Percent threshold: %f",  ((double)size_used/mei.me_mapsize) , resize_percent);
    
    if (threshold_size > 0) {
        if (mei.me_mapsize - size_used < threshold_size) {
            g_debug("Threshold met (size-based)");
            return true;
        } else {
            return false;
        }
    }
    
    if ((double)size_used / mei.me_mapsize  > resize_percent) {
        g_debug("Threshold met (percent-based)");
        return true;
    }
    return false;
#else
    return false;
#endif
}

void lmdb_do_resize(BlockchainLMDB *lmdb, uint64_t increase_size) {
    g_debug("BlockchainLMDB#lmdb_do_resize");
    const uint64_t add_size = 1LL << 30;
    // check disk capacity
    long available_space = get_available_space(lmdb->m_folder);
    if (available_space < add_size) {
        g_error("!! WARNING: Insufficient free space to extend database : %ld MB avaliable, %llu MB needed", (available_space >> 20L), (add_size >> 20L));
        return;
    }
    
    MDB_envinfo mei;
    
    mdb_env_info(lmdb->m_env, &mei);
    
    MDB_stat mst;
    
    mdb_env_stat(lmdb->m_env, &mst);
    
    // add 1Gb per resize, instead of doing a percentage increase
    uint64_t new_mapsize = (double) mei.me_mapsize + add_size;
    
    // If given, use increase_size instead of above way of resizing.
    // This is currently used for increasing by an estimated size at start of new
    // batch txn.
    if (increase_size > 0) {
        new_mapsize = mei.me_mapsize + increase_size;
    }
    
    new_mapsize += (new_mapsize % mst.ms_psize);
    
    //TODO impl
    mdb_txn_safe_prevent_new_txns();
    
    if (lmdb->m_write_txn != NULL) {
        if (lmdb->m_batch_active) {
            g_error("lmdb resizing not yet supported when batch transactions enabled!");
        } else {
            g_error("attempting resize with write transaction in progress, this should not happen!");
        }
    }
    //TODO impl
    mdb_txn_safe_wait_no_active_txns();
    
    int result = mdb_env_set_mapsize(lmdb->m_env, new_mapsize);
    if (result) {
        g_error("Failed to set new mapsize: %d", result);
        return;
    }
    
    g_info("LMDB Mapsize increased. Old: %luMiB, New: %lluMiB.", mei.me_mapsize / (1024 * 1024), new_mapsize / (1024 * 1024));
    
    mdb_txn_safe_allow_new_txns();
    
}

void lmdb_migrate(BlockchainLMDB *lmdb, const uint32_t oldversion) {
    g_error("not support migrate now!");
}
