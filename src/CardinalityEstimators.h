#ifndef _CARDINALITY_ESTIMATORS_H
#define _CARDINALITY_ESTIMATORS_H

#include <vector>
#include <string>
#include <queue>
#include <stdint.h>
#include "Serializer.h"

class ICardinalityEstimator {
    public:
        virtual ~ICardinalityEstimator() {}
        virtual void increment(const char *key, int len=-1) = 0;
        virtual int count() = 0;
        virtual std::string repr() = 0;
        virtual void merge_from(ICardinalityEstimator *other) = 0;
        virtual ICardinalityEstimator* clone() = 0;
        virtual void serialize(Serializer *serializer) = 0;
        virtual void unserialize(Serializer *serializer) = 0;

};

class HashingCardinalityEstimator: public ICardinalityEstimator {
    protected:
        uint64_t hash(const char *key);
        uint64_t hash(const char *key, int len);
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
        virtual void increment(const char *key, int len=-1);
        virtual int count();
        virtual std::string repr();
        virtual void merge_from(ICardinalityEstimator *other);
        virtual ICardinalityEstimator* clone();
        virtual void serialize(Serializer *serializer);
        virtual void unserialize(Serializer *serializer);
};

/* K Minimal Values estimator
 *
 * Based on http://blog.aggregateknowledge.com/2012/07/09/sketch-of-the-day-k-minimum-values/
 */
class KMinValuesCounter: public HashingCardinalityEstimator {
    protected:
        std::priority_queue<uint64_t> _minimal_values;
        int get_real_k();
        int k;
    public:
        /* k: number of minimal values to store. On the order of couple thousand. The more, the greater counting precision you get */
        KMinValuesCounter(int k);
        virtual void increment(const char *key, int len=-1);
        virtual int count();
        virtual std::string repr();
        virtual void merge_from(ICardinalityEstimator *other);
        virtual ICardinalityEstimator* clone();
        virtual void serialize(Serializer *serializer);
        virtual void unserialize(Serializer *serializer);
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
        virtual void increment(const char *key, int len=-1);
        virtual int count();
        virtual std::string repr();
        virtual void merge_from(ICardinalityEstimator *other);
        virtual ICardinalityEstimator* clone();
        virtual void serialize(Serializer *serializer);
        virtual void unserialize(Serializer *serializer);
};

class HyperLogLogOwnArrayCounter: public HashingCardinalityEstimator {
    protected:
        uint32_t *buckets[2];
        bool own_buckets_memory;
        int b;
        int m;
        int m_mask;
        double get_alpha();
        int number_of_zero_buckets();
    public:
        /* k: number of bits to use as bucket key. In the range of 4..16. The more, the greater counting precision you get */
        HyperLogLogOwnArrayCounter(int b, char *storage_region1, char *storage_region2);
        virtual ~HyperLogLogOwnArrayCounter();
        virtual void increment(const char *key, int len=-1);
        virtual int count();
        virtual std::string repr();
        virtual void merge_from(ICardinalityEstimator *other);
        virtual ICardinalityEstimator* clone();
        virtual void serialize(Serializer *serializer);
        virtual void unserialize(Serializer *serializer);
};

/* Dummy estimator
 *
 */
class DummyCounter: public HashingCardinalityEstimator {
    protected:
        int c;
    public:
        DummyCounter(int ignored);
        virtual void increment(const char *key, int len=-1);
        virtual int count();
        virtual std::string repr();
        virtual void merge_from(ICardinalityEstimator *other);
        virtual ICardinalityEstimator* clone();
        virtual void serialize(Serializer *serializer);
        virtual void unserialize(Serializer *serializer);
};

#endif
