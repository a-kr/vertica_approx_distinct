#ifndef _LINEAR_PROB_COUNTER_H
#define _LINEAR_PROB_COUNTER_H

#include <vector>
#include <stdint.h>


/*
 * Probabilistic distinct elements counter
 * which uses a fixed amount of memory.
 *
 * Based on https://gist.github.com/devdazed/3873524
 *
 */
class LinearProbabilisticCounter {
    protected:
        std::vector<bool> _bitset;
        int size_in_bits;
    public:
        /* size: number of bits in bitset. Should be on the order of couple millions. The more, the greater counting precision you get */
        LinearProbabilisticCounter(int size);
        void increment(char *key);
        int count_set_bits();
        uint64_t hash(char *key);
        int count();
};

#endif
