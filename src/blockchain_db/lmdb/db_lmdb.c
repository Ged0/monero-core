#include <stdio.h>

#include "db_lmdb.h"


void lmdb_open(BlockchainLMDB *lmdb, const char* filename, const int db_flags) {
    int result;
    int mdb_flags = MDB_NORDAHEAD;

    if (lmdb != NULL && lmdb->db->m_open) {
        /* code */
        printf("Attempted to open db, but it's already open");
        return;
    }
    
    //TODO check if data exists
    
    
}
