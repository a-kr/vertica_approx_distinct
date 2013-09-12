#ifndef _CARDINALITY_ESTIMATORS_H
#define _CARDINALITY_ESTIMATORS_H

#include <vector>
#include <stdint.h>
#include <string>

class ICardinalityEstimator {
    public:
        virtual ~ICardinalityEstimator() {}
        virtual void increment(char *key) = 0;
        virtual int count() = 0;
        virtual std::string repr() = 0;
};

class HashingCardinalityEstimator: public ICardinalityEstimator {
    protected:
        uint64_t hash(char *key);
};


/*
 * Probabilistic distinct elements counter
 * which uses a fixed amount of memory.
 *
 * Based on https://gist.github.com/devdazed/3873524
 *
 */
class LinearProbabilisticCounter: public HashingCardinalityEstimator {
    protected:
        std::vector<bool> _bitset;
        int size_in_bits;
        int count_set_bits();
    public:
        /* size: number of bits in bitset. Should be on the order of couple millions. The more, the greater counting precision you get */
        LinearProbabilisticCounter(int size);
        virtual void increment(char *key);
        virtual int count();
        virtual std::string repr();
};

#endif
