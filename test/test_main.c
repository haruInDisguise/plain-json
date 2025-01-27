#define PLAIN_JSON_IMPLEMENTATION
#include <plain_json.h>

#define TEST_IMPLEMENTATION
#include <test/test.h>

#include <fcntl.h>

#include <sys/io.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    if(! test_init(argc, argv)) {
        return -1;
    }
    test_run_all();
    test_exit();
    return 0;
}
