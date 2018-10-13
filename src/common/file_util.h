#ifndef MONERO_COMMON_FILE_UTIL_H_
#define MONERO_COMMON_FILE_UTIL_H_

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdbool.h>


bool is_dir_exists(char* filename);

bool is_file_exists(char* filename);

long get_available_space(const char* path);

#endif //MONERO_COMMON_FILE_UTIL_H_
