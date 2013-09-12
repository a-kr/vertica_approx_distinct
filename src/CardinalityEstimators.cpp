#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdint.h>

#include "MurmurHash3.h"
#include "CardinalityEstimators.h"

/******** HashingCardinalityEstimator ********/

uint64_t HashingCardinalityEstimator::hash(char *key) {
    int key_len = strlen(key);
    uint64_t h[2];
    MurmurHash3_x64_128(key, key_len, 0, &h[0]);
    return h[0];
}

/******* LinearProbabilisticCounter ********/

LinearProbabilisticCounter::LinearProbabilisticCounter(int size): _bitset(size, false) {
    this->size_in_bits = size;
}

void LinearProbabilisticCounter::increment(char *key) {
    uint64_t h = this->hash(key);
    uint64_t i = h % this->size_in_bits;
    this->_bitset[i] = true;
}

int LinearProbabilisticCounter::count_set_bits() {
    return (int)std::count(
        this->_bitset.begin(),
        this->_bitset.end(),
        true
    );
}

int LinearProbabilisticCounter::count() {
    /* -size * ln(unset_bits/size) */
    int unset_bits = this->size_in_bits - this->count_set_bits();
    if (unset_bits == 0) {
        return this->size_in_bits;
    }
    double ratio = double(unset_bits) / double(this->size_in_bits);
    return -this->size_in_bits * log(ratio);
}

std::string LinearProbabilisticCounter::repr() {
    char buf[50];
    sprintf(buf, "LinearProbabilisticCounter(n=%d)", this->size_in_bits);
    return std::string(buf);
}
