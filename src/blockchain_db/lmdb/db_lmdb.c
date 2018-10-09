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
        //TODO  if mkdir failed, return.
        if ((result = mkdir(filename, 0777))) {
            g_error("Create file failed, filename: %s, result: %d", filename, result);
            return;
        }
    }
    if (!S_ISDIR(sb.st_mode)) {
        g_error("LMDB needs a directory path, but a file was passed");
        return;
    }
    
    int index = strrchr(filename, '/');
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
    
}
