#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "CardinalityEstimators.h"

void test(int n_elements) {
    char buf[50];
    int i, c;
    std::vector<ICardinalityEstimator*> counters;

    //counters.push_back(new LinearProbabilisticCounter(16 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(256 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(1 * 1024 * 1024 * 8));
    //counters.push_back(new LinearProbabilisticCounter(4 * 1024 * 1024 * 8));
    //counters.push_back(new KMinValuesCounter(128));
    //counters.push_back(new KMinValuesCounter(256));
    counters.push_back(new KMinValuesCounter(1024));
    //counters.push_back(new HyperLogLogCounter(8));
    counters.push_back(new HyperLogLogCounter(12));
    counters.push_back(new HyperLogLogCounter(16));

    printf("Testing with %d elements...\n", n_elements);

    for (i = 0; i < n_elements; i++) {
        sprintf(buf, "%u", i);
        for (c = 0; c < (int)counters.size(); c++) {
            counters[c]->increment(buf);
        }
    }

    while (counters.size() > 0) {
        ICardinalityEstimator *counter = counters.back();
        int count = counter->count();
        double err_percent = 100.0 * abs(double(count) - n_elements) / double(n_elements);
        printf("%s:\tcount = %d (error = %.2f%%)\n", counter->repr().c_str(), count, err_percent);
        delete counter;
        counters.pop_back();
    }
}

void count_stdin(ICardinalityEstimator *counter) {
    int i = 0;
    char *line = NULL;
    size_t line_size;
    while (getline(&line, &line_size, stdin) != -1) {
        counter->increment(line);
        i++;
    }
    printf("i: %d\tcount: %d\n", i, counter->count());
}

int main(int argc, char **argv) {
    int size = 0;

    test(100);
    test(1000);
    test(10000);
    test(100000);
    test(1000000);
    return 0;

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
    count_stdin(counter);
    delete counter;
}



