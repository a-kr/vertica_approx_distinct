#ifndef _SERIALIZER_H
#define _SERIALIZER_H

#include <vector>
#include <stdexcept>

class Serializer {
    protected:
        std::vector<char *> storage;
        std::vector<size_t> storage_size;
        size_t storage_i;
        size_t storage_pos;

        void check_remaining_space_and_switch_container_if_needed(size_t required_space) {
            if (this->storage.size() > 0) {
                if (this->storage_pos + required_space < this->storage_size[this->storage_pos]) {
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
            throw std::runtime_error("not enough space");
        }

    public:
        Serializer(): storage(), storage_size() {
            this->storage_i = 0;
            this->storage_pos = 0;
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

        void write(int x) {
            this->write((char *)&x, (size_t)sizeof(x));
        }

        void write(uint64_t x) {
            this->write((char *)&x, (size_t)sizeof(x));
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
};

#endif
