#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include "MurmurHash3.h"
#include "LinearProbCounter.h"


LinearProbabilisticCounter::LinearProbabilisticCounter(int size): _bitset(size, false) {
    this->size_in_bits = size;
}

uint64_t LinearProbabilisticCounter::hash(char *key) {
    int key_len = strlen(key);
    uint64_t h[2];
    MurmurHash3_x64_128(key, key_len, 0, &h[0]);
    return h[0];
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
