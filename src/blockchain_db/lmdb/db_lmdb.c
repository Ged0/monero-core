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
        printf("Attempted to open db, but it's already open");
        return;
    }
    
    //TODO check if data exists
    struct stat sb;
    if (stat(filename, &sb) != 0) {
        //TODO  if mkdir failed, return.
        int mkdirResult = mkdir(filename, 0777);
    }
    if (!S_ISDIR(sb.st_mode)) {
        printf("LMDB needs a directory path, but a file was passed");
        return;
    }
    
    int index = strrchr(filename, '/');
    char oldFiles[index+1];
    strncpy(oldFiles, filename, index+1);
    
    
    
}
