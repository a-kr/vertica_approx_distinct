#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <sys/time.h>
#include "CardinalityEstimators.h"

void merging_test(ICardinalityEstimator *base_counter) {
    int n_elements = 1000000;
    char buf[50];
    int i;
    int num_counters = 100;
    std::vector<ICardinalityEstimator*> counters;

    counters.push_back(base_counter);
    for (i = 1; i < num_counters; i++) {
        counters.push_back(base_counter->clone());
    }

    printf("Testing with %d elements...\n", n_elements);

    for (i = 0; i < n_elements; i++) {
        ICardinalityEstimator *counter = counters[i % num_counters];
        sprintf(buf, "%u", i);
        counter->increment(buf);
    }

    for (i = 1; i < num_counters; i++) {
        counters[0]->merge_from(counters[i]);
        delete counters[i];
    }

    int count = counters[0]->count();
    double err_percent = 100.0 * abs(double(count) - n_elements) / double(n_elements);
    printf("%s:\tcount = %d (error = %.2f%%)\n", counters[0]->repr().c_str(), count, err_percent);
    delete counters[0];
}


void benchmark() {
    int n_elements = 10000000;
    char buf[50];
    int i;
    struct timeval t0, t1;
    double dt;
    std::vector<ICardinalityEstimator*> counters;

    counters.push_back(new LinearProbabilisticCounter(128 * 1024 * 8));
    counters.push_back(new KMinValuesCounter(16 * 1024));
    counters.push_back(new HyperLogLogCounter(15));
    counters.push_back(new DummyCounter());

    printf("Testing with %d elements...\n", n_elements);

    while (counters.size() > 0) {
        ICardinalityEstimator *counter = counters.back();
        gettimeofday(&t0, NULL);
        for (i = 0; i < n_elements; i++) {
            sprintf(buf, "%u", i);
            counter->increment(buf);
        }
        int count = counter->count();
        gettimeofday(&t1, NULL);
        dt = (t1.tv_sec - t0.tv_sec) + (double(t1.tv_usec - t0.tv_usec) / 1000000.0);
        double err_percent = 100.0 * abs(double(count) - n_elements) / double(n_elements);
        printf("%s:\tcount = %d (error = %.2f%%) time = %.3fs\n", counter->repr().c_str(), count, err_percent, dt);
        delete counter;
        counters.pop_back();
    }
}

void test(int n_elements) {
    char buf[50];
    int i, c;
    std::vector<ICardinalityEstimator*> counters;

    counters.push_back(new LinearProbabilisticCounter(16 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(32 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(64 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(128 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(256 * 1024 * 8));
    counters.push_back(new LinearProbabilisticCounter(1 * 1024 * 1024 * 8));
    //counters.push_back(new LinearProbabilisticCounter(4 * 1024 * 1024 * 8));
    //counters.push_back(new KMinValuesCounter(128));
    //counters.push_back(new KMinValuesCounter(256));
    //counters.push_back(new KMinValuesCounter(1024));
    //counters.push_back(new HyperLogLogCounter(6));
    counters.push_back(new HyperLogLogCounter(12));
    counters.push_back(new HyperLogLogCounter(13));
    counters.push_back(new HyperLogLogCounter(14));
    counters.push_back(new HyperLogLogCounter(15));
    counters.push_back(new HyperLogLogCounter(16));
    counters.push_back(new HyperLogLogCounter(18));

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

    //merging_test(new LinearProbabilisticCounter(128 * 1024 * 8));
    //merging_test(new KMinValuesCounter(1024));
    //merging_test(new HyperLogLogCounter(16));

    benchmark();
    return 0;

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



