#include <cstdlib>
#include <cstdio>
#include "CardinalityEstimators.h"


int main(int argc, char **argv) {
    int size = 0;
    if (argc == 2) {
        size = atoi(argv[1]) * 1024 * 1024 * 8;
        if (size == 0) {
            printf("Please specify non-zero memory size!\n");
            return 0;
        }
    } else {
        printf("Usage: cat SOMEFILE | ./test_main <bitset size in megabytes>\n");
        return 0;
    }
    LinearProbabilisticCounter *counter = new LinearProbabilisticCounter(size);

    int i = 0;
    char *line = NULL;
    size_t line_size;
    while (getline(&line, &line_size, stdin) != -1) {
        counter->increment(line);
        i++;
    }
    printf("i: %d\tset bits: %d\tcount: %d\n", i, counter->count_set_bits(), counter->count());
}



