#include <sys/stat.h>
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

