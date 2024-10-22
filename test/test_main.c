#include <fcntl.h>

#include <sys/io.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define TEST_IMPLEMENTATION
#include <test/test.h>

#define JSON_IMPLEMENTATION
#include "../json.h"

int main(int argc, char **argv) {
    test_init(argc, argv);
    test_run_all();
    return 0;
}
