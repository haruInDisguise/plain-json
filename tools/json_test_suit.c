#define PLAIN_JSON_OPTION_DEPTH 32

#define PLAIN_JSON_IMPLEMENTATION
#include "../plain_json.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>

typedef void (*foreach_func)(const char *file_path);

void foreach_in_dir(const char* path, foreach_func func) {
    DIR *dir_ptr = opendir(path);
    if(dir_ptr == NULL) {
        perror("Failed to open dir");
    }

    struct dirent *entry_ptr;
    while((entry_ptr = readdir(dir_ptr)) != NULL) {
        func(entry_ptr->d_name);
    }
}

int main(void) {
    const char *path = "JSONTestSuite/test_parsing";

    return 0;
}
