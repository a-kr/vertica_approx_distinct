#ifndef _CARDINALITY_ESTIMATORS_H
#define _CARDINALITY_ESTIMATORS_H

#include <vector>
#include <string>
#include <queue>
#include <stdint.h>

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
 * Linear probabilistic counter.
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

/* A comparator for sorting in ascending order */
class MinHeapComparator {
    public:
        bool operator() (uint64_t a, uint64_t b) {
            return a > b;
        }
};

/* K Minimal Values estimator
 *
 * Based on http://blog.aggregateknowledge.com/2012/07/09/sketch-of-the-day-k-minimum-values/
 */
class KMinValuesCounter: public HashingCardinalityEstimator {
    protected:
        std::vector<uint64_t> _minimal_values;
        int _max_of_min_i;
        int _values_stored;
        uint64_t _max_of_min;
        void save_value_and_recalculate_max(uint64_t value);
        int k;
    public:
        /* k: number of minimal values to store. On the order of couple hundred. The more, the greater counting precision you get */
        KMinValuesCounter(int k);
        virtual void increment(char *key);
        virtual int count();
        virtual std::string repr();
};

/* HyperLogLog estimator
 *
 * Based on https://github.com/JonJanzen/hyperloglog/blob/master/hyperloglog/hll.py
 * and http://blog.aggregateknowledge.com/2012/10/25/sketch-of-the-day-hyperloglog-cornerstone-of-a-big-data-infrastructure/
 */
class HyperLogLogCounter: public HashingCardinalityEstimator {
    protected:
        std::vector<int> buckets;
        int b;
        int m;
        int m_mask;
        double get_alpha();
        int number_of_zero_buckets();
    public:
        /* k: number of bits to use as bucket key. In the range of 4..16. The more, the greater counting precision you get */
        HyperLogLogCounter(int b);
        virtual void increment(char *key);
        virtual int count();
        virtual std::string repr();
};

/* Dummy estimator
 *
 */
class DummyCounter: public HashingCardinalityEstimator {
    protected:
        int c;
    public:
        DummyCounter();
        virtual void increment(char *key);
        virtual int count();
        virtual std::string repr();
};

#endif
