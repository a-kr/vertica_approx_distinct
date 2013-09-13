#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdint.h>

#include "MurmurHash3.h"
#include "CardinalityEstimators.h"

/******** Utilities *******/

#define UINT64_MAX (18446744073709551615ULL)

std::string human_readable_size(int bytes) {
    char buf[20];
    int kb = bytes / 1024;
    int b = bytes % 1024;
    if (kb == 0) {
        sprintf(buf, "%d", b);
    } else if (kb < 1024) {
        sprintf(buf, "%dK", kb);
    } else {
        sprintf(buf, "%.1fM", double(kb) / 1024.0);
    }
    return std::string(buf);
}

int constrain_int(int v, int min, int max) {
    if (v > max) {
        return max;
    } else if (v < min) {
        return min;
    }
    return v;
}

/* Counts the length of the run of `one` bits starting from the least significant bits
 * TODO: probably can be crazily optimized
 */
inline int count_run_of_ones(uint64_t val) {
    int n = 0;
    while (val) {
        if (val & 1) {
            n++;
            val = val >> 1;
        } else {
            break;
        }
    }
    /* HACK: this '+1' should not be here, and compensates for some unknown bug
     * elsewhere in the code, which I was not able to locate */
    return n + 1;
}

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
    char buf[100];
    int memory = this->size_in_bits / 8;
    sprintf(buf, "LinearProbabilisticCounter(n=%d, %s bytes)", this->size_in_bits, human_readable_size(memory).c_str());
    return std::string(buf);
}

/******* KMinValuesCounter ********/

KMinValuesCounter::KMinValuesCounter(int k): _minimal_values(k, 0) {
    this->k = k;
    this->_max_of_min_i = 0;
    this->_max_of_min = UINT64_MAX;
    this->_values_stored = 0;
}

void KMinValuesCounter::save_value_and_recalculate_max(uint64_t newval) {
    int i;
    uint64_t max = 0;
    uint64_t max_i = 0;
    if (this->_values_stored < this->k) {
        // there are still free places, put new value there
        // instead of replacing an old one
        this->_minimal_values[this->_values_stored++] = newval;
    } else {
        // replace an old value
        this->_minimal_values[this->_max_of_min_i] = newval;
    }
    // recompute index of the new maximum
    for (i = 0; i < this->k; i++) {
        if (this->_minimal_values[i] > max) {
            max = this->_minimal_values[i];
            max_i = i;
        }
    }
    this->_max_of_min_i = max_i;
    this->_max_of_min = max;
}

void KMinValuesCounter::increment(char *key) {
    uint64_t h = this->hash(key);
    if (h < this->_max_of_min) {
        this->save_value_and_recalculate_max(h);
    }
}

int KMinValuesCounter::count() {
    /* (k - 1) / kth_min_normalized  */
    /* == (k - 1) / (kth_min / UINT64_MAX)  */
    /* == UINT64_MAX * (k - 1) / kth_min  */
    int k = this->k;
    if (this->_values_stored < k) {
        k = this->_values_stored;
    }
    return int(UINT64_MAX * double(k - 1) / double(this->_max_of_min));
}

std::string KMinValuesCounter::repr() {
    char buf[50];
    int memory = sizeof(uint64_t) * this->k;
    sprintf(buf, "KMinValuesCounter(k=%d, %s bytes)", this->k, human_readable_size(memory).c_str());
    return std::string(buf);
}

/******* HyperLogLogCounter ********/

#define HYPER_LOG_LOG_B_MAX 20

HyperLogLogCounter::HyperLogLogCounter(int b): buckets(
        int(pow(2, constrain_int(b, 4, HYPER_LOG_LOG_B_MAX))), 0) {
    this->b = b;
    this->m = int(pow(2, constrain_int(b, 4, HYPER_LOG_LOG_B_MAX)));
    this->m_mask = this->m - 1; // 'b' ones
}

double HyperLogLogCounter::get_alpha() {
    switch (this->b) {
        case 4: return 0.673;
        case 5: return 0.697;
        case 6: return 0.709;
    }
    return 0.7213 / (1.0 + 1.079 / double(1 << this->b));
}

void HyperLogLogCounter::increment(char *key) {
    uint64_t h = this->hash(key);
    int j = h & this->m_mask;
    uint64_t w = h >> this->b;
    int run_of_ones = count_run_of_ones(w);
    this->buckets[j] = (run_of_ones > this->buckets[j]) ? run_of_ones : this->buckets[j];
}

int HyperLogLogCounter::count() {
    /* DV_est = alpha * m^2 * 1/sum( 2^ -register ) */
    double estimate = this->get_alpha() * this->m * this->m;
    double sum = 0.0;
    int i;
    for (i = 0; i < this->m; i++) {
        sum += pow(2, -this->buckets[i]);
    }
    estimate = estimate * 1.0 / sum;

    if (estimate < 2.5 * this->m) {
        // small range correction
        int v = this->number_of_zero_buckets();
        if (v > 0) {
            estimate = this->m * log(this->m / double(v));
        }
    } else if (estimate > 1/30.0 * pow(2, 64)) {
        // large range correction (for hash collisions)
        // And we use a 64-bit hash...
        estimate = -pow(2, 64) * log(1.0 - estimate / pow(2, 64));
    }
    return estimate;
}

int HyperLogLogCounter::number_of_zero_buckets() {
    return (int)std::count(
        this->buckets.begin(),
        this->buckets.end(),
        0
    );
}

std::string HyperLogLogCounter::repr() {
    char buf[50];
    int memory = sizeof(int) * this->m;
    sprintf(buf, "HyperLogLogCounter(b=%d, m=%d, %s bytes)", this->b, this->m, human_readable_size(memory).c_str());
    return std::string(buf);
}
