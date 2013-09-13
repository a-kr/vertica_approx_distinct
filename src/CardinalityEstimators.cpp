#include <vector>
#include <algorithm>
#include <stdexcept>
#include <queue>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <stdint.h>

#include "MurmurHash3.h"
#include "CardinalityEstimators.h"

/******** Utilities *******/

#define UINT64_MAX (18446744073709551615ULL)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

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

void print_binary(uint64_t n) {
    int i;
    for (i = 63; i >= 0; i--) {
        if (n & (1 << i)) {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");
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

void LinearProbabilisticCounter::merge_from(ICardinalityEstimator *that) {
    LinearProbabilisticCounter *other = (LinearProbabilisticCounter *)that;
    int i;
    if (this->size_in_bits != other->size_in_bits) {
        throw std::runtime_error("cannot merge LinearProbabilisticCounters with different parameters");
    }
    for (i = 0; i < this->size_in_bits; i++) {
        this->_bitset[i] = this->_bitset[i] || other->_bitset[i];
    }
}

ICardinalityEstimator* LinearProbabilisticCounter::clone() {
    return new LinearProbabilisticCounter(this->size_in_bits);
}

void LinearProbabilisticCounter::serialize(Serializer *serializer) {
    serializer->write_int(this->size_in_bits);
    uint64_t bit_buffer = 0;
    int bb_i = 0;
    int bit_i;
    for (bit_i = 0; bit_i < this->size_in_bits; bit_i++) {
        bit_buffer |= ((uint64_t)this->_bitset[bit_i] << bb_i);
        bb_i++;
        if (unlikely(bb_i >= 64)) {
            serializer->write_uint64_t(bit_buffer);
            bit_buffer = 0;
            bb_i = 0;
        }
    }
    if (bb_i > 0) {
        serializer->write_uint64_t(bit_buffer);
    }
}

void LinearProbabilisticCounter::unserialize(Serializer *serializer) {
    this->size_in_bits = serializer->read_int();
    this->_bitset.resize(this->size_in_bits, false);
    uint64_t bit_buffer = 0;
    int bb_i = 64; // to force read on the first iteration
    int bit_i = 0;
    for (bit_i = 0; bit_i < this->size_in_bits; bit_i++) {
        if (unlikely(bb_i >= 64)) {
            bit_buffer = serializer->read_uint64_t();
            bb_i = 0;
        }
        this->_bitset[bit_i] = !!(bit_buffer & ((uint64_t)1 << bb_i));
        bb_i++;
    }
}

/******* KMinValuesCounter ********/

KMinValuesCounter::KMinValuesCounter(int k) : _minimal_values() {
    this->k = k;
}

int KMinValuesCounter::get_real_k() {
    if ((int)this->_minimal_values.size() < this->k) {
        return this->k;
    }
    return this->_minimal_values.size();
}

void KMinValuesCounter::increment(char *key) {
    uint64_t h = this->hash(key);
    if (unlikely((int)this->_minimal_values.size() < this->k)) {
        this->_minimal_values.push(h);
    } else if (unlikely(h < this->_minimal_values.top())) {
        if (likely((int)this->_minimal_values.size() == this->k)) {
            this->_minimal_values.pop();
        this->_minimal_values.push(h);
        }
    }
}

int KMinValuesCounter::count() {
    /* (k - 1) / kth_min_normalized  */
    /* == (k - 1) / (kth_min / UINT64_MAX)  */
    /* == UINT64_MAX * (k - 1) / kth_min  */
    int k = this->get_real_k();
    if (k == 0) {
        return 0;
    }
    return int(UINT64_MAX * double(k - 1) / double(this->_minimal_values.top()));
}

std::string KMinValuesCounter::repr() {
    char buf[50];
    int memory = sizeof(uint64_t) * this->k;
    sprintf(buf, "KMinValuesCounter(k=%d, %s bytes)", this->k, human_readable_size(memory).c_str());
    return std::string(buf);
}

/* Caution: this is a destructive merge, i.e. it empties "that" counter */
void KMinValuesCounter::merge_from(ICardinalityEstimator *that) {
    KMinValuesCounter *other = (KMinValuesCounter *)that;
    uint64_t v;
    while (!other->_minimal_values.empty()) {
        v = other->_minimal_values.top();
        other->_minimal_values.pop();
        if (v < this->_minimal_values.top()) {
            if ((int)this->_minimal_values.size() == this->k) {
                this->_minimal_values.pop();
            }
            this->_minimal_values.push(v);
        }
    }
}

ICardinalityEstimator* KMinValuesCounter::clone() {
    return new KMinValuesCounter(this->k);
}

void KMinValuesCounter::serialize(Serializer *serializer) {
    std::vector<uint64_t> backup_for_queue;
    serializer->write_int(this->k);
    serializer->write_int(this->_minimal_values.size());

    while (!this->_minimal_values.empty()) {
        uint64_t v = this->_minimal_values.top();
        this->_minimal_values.pop();
        backup_for_queue.push_back(v);
        serializer->write_uint64_t(v);
    }

    while (!backup_for_queue.empty()) {
        uint64_t v = backup_for_queue.back();
        backup_for_queue.pop_back();
        this->_minimal_values.push(v);
    }
}

void KMinValuesCounter::unserialize(Serializer *serializer) {
    this->k = serializer->read_int();
    int n = serializer->read_int();
    // strangely enough, priority_queue has no method clear()
    while (!this->_minimal_values.empty()) {
        this->_minimal_values.pop();
    }
    for (int i = 0; i < n; i++) {
        uint64_t v = serializer->read_uint64_t();
        //printf("restoring %lu\n", v);
        this->_minimal_values.push(v);
    }
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

void HyperLogLogCounter::merge_from(ICardinalityEstimator *that) {
    HyperLogLogCounter *other = (HyperLogLogCounter *)that;
    if (this->m != other->m) {
        throw std::runtime_error("cannot merge HyperLogLogCounters with different parameters");
    }
    int i;
    for (i = 0; i < this->m; i++) {
        int my_v = this->buckets[i];
        int his_v = other->buckets[i];
        this->buckets[i] = (my_v > his_v) ? my_v : his_v;
    }
}

ICardinalityEstimator* HyperLogLogCounter::clone() {
    return new HyperLogLogCounter(this->b);
}

void HyperLogLogCounter::serialize(Serializer *serializer) {
    serializer->write_int(this->b);
    serializer->write_int(this->m);
    serializer->write_int(this->m_mask);
    for (int i = 0; i < this->m; i++) {
        serializer->write_int(this->buckets[i]);
    }
}

void HyperLogLogCounter::unserialize(Serializer *serializer) {
    this->b = serializer->read_int();
    this->m = serializer->read_int();
    this->m_mask = serializer->read_int();
    this->buckets.resize(this->m);
    for (int i = 0; i < this->m; i++) {
        int v = serializer->read_int();
        this->buckets[i] = v;
    }
}

/******* DummyCounter ********/

DummyCounter::DummyCounter() {
    this->c = 0;
}

void DummyCounter::increment(char *key) {
    this->c++;
}

int DummyCounter::count() {
    return this->c;
}

std::string DummyCounter::repr() {
    char buf[50];
    int memory = sizeof(int);
    sprintf(buf, "DummyCounter(%s bytes)", human_readable_size(memory).c_str());
    return std::string(buf);
}

void DummyCounter::merge_from(ICardinalityEstimator *that) {
    DummyCounter *other = (DummyCounter *)that;
    this->c += other->c;
}

ICardinalityEstimator* DummyCounter::clone() {
    return new DummyCounter();
}

void DummyCounter::serialize(Serializer *serializer) {
    serializer->write_int(this->c);
}

void DummyCounter::unserialize(Serializer *serializer) {
    this->c = serializer->read_int();
}
