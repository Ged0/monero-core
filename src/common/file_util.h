#ifndef MONERO_COMMON_FILE_UTIL_H_
#define MONERO_COMMON_FILE_UTIL_H_

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdbool.h>


bool is_dir_exists(char* filename) {
    struct stat sb;
    if (stat(filename, &sb) < 0) {
        return false;
    }
    if (!S_ISDIR(sb.st_mode)) {
        return false;
    }
    return true;
}

bool is_file_exists(char* filename) {
    struct stat sb;
    if (stat(filename, &sb) < 0) {
        return false;
    }
    if (!S_ISREG(sb.st_mode)) {
        return false;
    }
    return true;
}

long get_available_space(const char* path) {
    struct statvfs stat;
    
    if (statvfs(path, &stat) != 0) {
        // error happens, just quits here
        return -1;
    }
    
    // the available size is f_bsize * f_bavail
    return stat.f_bsize * stat.f_bavail;
}

#endif //MONERO_COMMON_FILE_UTIL_H_
