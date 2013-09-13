#ifndef _SERIALIZER_H
#define _SERIALIZER_H

#include <vector>
#include <stdexcept>
#include <cstdio>

class Serializer {
    protected:
        std::vector<char *> storage;
        std::vector<size_t> storage_size;
        size_t storage_i;
        size_t storage_pos;

        void check_remaining_space_and_switch_container_if_needed(size_t required_space) {
            if (this->storage.size() > 0) {
                if (this->storage_pos + required_space < this->storage_size[this->storage_i]) {
                    return;
                }
                // not enough space in current container
                if (this->storage.size() > this->storage_i + 1) {
                    // advancing to a new shiny container
                    this->storage_i += 1;
                    this->storage_pos = 0;
                    return;
                }
            }
            char buf[100];
            sprintf(buf, "Not enough space: size %lu, capacity %lu, requested %lu",
                    this->size(), this->capacity(), required_space);
            throw std::runtime_error(buf);
        }

    public:
        Serializer(): storage(), storage_size() {
            this->storage_i = 0;
            this->storage_pos = 0;
        }

        void free_containers() {
            while (!this->storage.empty()) {
                char *data = this->storage.back();
                delete data;
                this->storage.pop_back();
                this->storage_size.pop_back();
            }
            this->storage_i = 0;
            this->storage_pos = 0;
        }

        size_t size() {
            size_t s = 0;
            for (int i = 0; i < (int)this->storage_i; i++) {
                s += this->storage_size[i];
            }
            s += this->storage_pos;
            return s;
        }

        size_t capacity() {
            size_t s = 0;
            for (int i = 0; i < (int)this->storage.size(); i++) {
                s += this->storage_size[i];
            }
            return s;
        }

        void add_storage(char *data, size_t len) {
            this->storage.push_back(data);
            this->storage_size.push_back(len);
        }

        void reset() {
            this->storage_i = 0;
            this->storage_pos = 0;
        }

        bool eof() {
            if (this->storage.size() == 0) return true;
            if (this->storage_i < this->storage.size() - 1) return false;
            if (this->storage_pos < this->storage_size[this->storage_i] - 1) return false;
            return true;
        }

        /* Warning: works only if data fits completely in the current storage container. */
        void write(char *data, size_t len) {
            this->check_remaining_space_and_switch_container_if_needed(len);
            size_t i;
            char *dest = this->storage[this->storage_i] + this->storage_pos;
            for (i = 0; i < len; i++) {
                *dest++ = *data++;
            }
            this->storage_pos += len;
        }

        void read(char *data, size_t len) {
            this->check_remaining_space_and_switch_container_if_needed(len);
            size_t i;
            char *src = this->storage[this->storage_i] + this->storage_pos;
            for (i = 0; i < len; i++) {
                *data++ = *src++;
            }
            this->storage_pos += len;
        }

        void write_int(int x) {
            this->write((char *)&x, (size_t)sizeof(int));
        }

        void write_uint64_t(uint64_t x) {
            this->write((char *)&x, (size_t)sizeof(uint64_t));
        }

        void write_uint32_t(uint32_t x) {
            this->write((char *)&x, (size_t)sizeof(uint32_t));
        }

        int read_int() {
            int r;
            this->read((char *)&r, (size_t)sizeof(int));
            return r;
        }

        uint64_t read_uint64_t() {
            uint64_t r;
            this->read((char *)&r, (size_t)sizeof(uint64_t));
            return r;
        }

        uint32_t read_uint32_t() {
            uint32_t r;
            this->read((char *)&r, (size_t)sizeof(uint32_t));
            return r;
        }
};

#endif
