#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include "common/file_util.h"
#include "db_lmdb.h"

void lmdb_open(BlockchainLMDB *lmdb, const char* filename, const int db_flags) {
    int result;
    int mdb_flags = MDB_NORDAHEAD;

    if (lmdb != NULL && lmdb->db->m_open) {
        /* code */
        g_error("Attempted to open db, but it's already open");
        return;
    }
    
    struct stat sb;
    if (stat(filename, &sb) != 0) {
        if ((result = mkdir(filename, 0777))) {
            g_error("Create file failed, filename: %s, result: %d", filename, result);
            return;
        }
    }
    if (!S_ISDIR(sb.st_mode)) {
        g_error("LMDB needs a directory path, but a file was passed");
        return;
    }
    
    int index = (int)(strrchr(filename, '/') - filename);
    char oldFiles[index+1];
    strncpy(oldFiles, filename, index+1);
    
    char block_data_file_path[strlen(oldFiles) + strlen(CRYPTONOTE_BLOCKCHAINDATA_FILENAME)];
    strcpy(block_data_file_path, oldFiles);
    strcat(block_data_file_path, CRYPTONOTE_BLOCKCHAINDATA_FILENAME);
    char block_lock_file_path[strlen(oldFiles) + strlen(CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME)];
    strcpy(block_lock_file_path, oldFiles);
    strcpy(block_lock_file_path, CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME);
    if (is_file_exists(block_data_file_path) || is_file_exists(block_lock_file_path)) {
        g_error("Found existing LMDB files in %s", oldFiles);
        g_error("Move %s and/or %s to %s, or delete them, and then restart", block_data_file_path, block_lock_file_path, oldFiles);
        return;
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
        g_error("Failed to create lmdb environment: %d", result);
        return;
    }
    
    if ((result = mdb_env_set_maxdbs(lmdb->m_env, 20))) {
        g_error("Failed to set max number of dbs: %d", result);
        return;
    }
    
    //TODO need to calculate from core of CPU
    int threads = 20;
    if (threads > 110
        && (result = mdb_env_set_maxreaders(lmdb->m_env, threads + 16))) {
        g_error("Failed to set max number of readers: %d", result);
        return;
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
        g_error("Failed to open lmdb environment: %d", result);
        return;
    }
    
    MDB_envinfo mei;
    mdb_env_info(lmdb->m_env, &mei);
    uint64_t cur_mapsize = (double)mei.me_mapsize;
    
    if (cur_mapsize < mapsize) {
        if ((result = mdb_env_set_mapsize(lmdb->m_env, mapsize))) {
            printf("Failed to set max memory map size: %d", result);
            return;
        }
        mdb_env_info(lmdb->m_env, &mei);
        cur_mapsize = (double)mei.me_mapsize;
        g_info("LMDB memory map size: %llu", cur_mapsize);
    }
    
    if (lmdb_need_resize(lmdb, 0)) {
        lmdb_do_resize(lmdb, 0);
    }
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
        g_error("!! WARNING: Insufficient free space to extend database !!: %ld MB avaliable, %llu MB needed", available_space >> 20L, add_size >> 20L);
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
    
    if (lmdb->m_write_txn != NULL) {
        if (lmdb->m_batch_active) {
            g_error("lmdb resizing not yet supported when batch transactions enabled!");
        } else {
            g_error("attempting resize with write transaction in progress, this should not happen!");
        }
    }

    
}
