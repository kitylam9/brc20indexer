#include <iostream>
#include <unordered_map> 
#include <fstream>
#include <string>
#include <vector> 
#include <chrono>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cassert> 
#include <leveldb/db.h> // leveldb::*
#include <leveldb/write_batch.h> // leveldb::WriteBatch
 
using namespace std;
class Hashtable {
    std::unordered_map<const void *, const void *> htmap;

public:
    void put(const void *key, const void *value) {
            htmap[key] = value;
    }

    const void *get(const void *key) {
            return htmap[key];
    }

};
 
const std::string INDEX_PATH = "blocks/index";
const uint64_t FIRST_INSCRIPTION_HEIGHT = 767430;
const size_t DEFAULT_INSCRIPTION_HEIGHT = 100000;
const size_t _DEFAULT_BLK_NUM = 10000;
const uint64_t BLOCK_VALID_CHAIN = 4;
const uint64_t BLOCK_HAVE_DATA = 8;

class BlkError : public std::exception {
public:
    BlkError(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class Status : public std::exception {
public:
    Status(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};
 
class BLK {
public:
    BLK(const std::string& btc_data_dir, uint64_t blk_index) {
        // implementation
    }
    void open() {
        // implementation
    }
    void close() {
        // implementation
    }
    Block read_block(uint64_t data_offset) {
        // implementation
    }
    // other methods
};

class Block {
    // implementation
};

class IndexError : public std::exception {
public:
    IndexError(const std::string& message) : message_(message) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class IndexEntry {
public:
    IndexEntry(const std::vector<uint8_t>& block_hash, uint64_t blk_index, uint64_t data_offset, uint64_t version, uint64_t height, uint64_t status, uint64_t tx_count)
        : block_hash_(block_hash), blk_index_(blk_index), data_offset_(data_offset), version_(version), height_(height), status_(status), tx_count_(tx_count) {}
    // getters
private:
    std::vector<uint8_t> block_hash_;
    uint64_t blk_index_;
    uint64_t data_offset_;
    uint64_t version_;
    uint64_t height_;
    uint64_t status_;
    uint64_t tx_count_;
};

class Index {
public:
    Index(const std::string& btc_data_dir) {
        // implementation
    }
    Block catch_block(uint64_t height) {
        // implementation
    }
    const IndexEntry* get_index_entry(uint64_t height) {
        // implementation
    }
    IndexEntry get_block_entry_by_block_hash(const std::vector<uint8_t>& block_hash) {
        // implementation
    }
private:
    std::string btc_data_dir_;
    std::unordered_map<uint64_t, IndexEntry> entries_;
    uint64_t max_height_;
    std::unordered_map<uint64_t, uint64_t> max_height_in_blk_;
    std::unordered_map<uint64_t, BLK> blks_;
};

bool is_block_index_entry(const std::vector<uint8_t>& data) {
    return data[0] == 'b';
}

uint64_t read_varint(std::vector<uint8_t>& reader) {
    uint64_t n = 0;
    size_t i = 0;
    while (true) {
        uint8_t ch_data = reader[i++];
        if (n > UINT64_MAX >> 7) {
            throw std::runtime_error("size too large");
        }
        n = (n << 7) | (ch_data & 0x7F);
        if (ch_data & 0x80) {
            if (n == UINT64_MAX) {
                throw std::runtime_error("size too large");
            }
            n += 1;
        } else {
            break;
        }
    }
    return n;
}

std::pair<std::unordered_map<uint64_t, IndexEntry>, uint64_t, std::unordered_map<uint64_t, uint64_t>, std::unordered_map<uint64_t, BLK>> parse_index_for_ordinals(const std::string& btc_data_dir) {
    std::string index_path = btc_data_dir + "/" + INDEX_PATH;
    if (!std::filesystem::exists(index_path)) {
        throw IndexError("Database index not found: " + index_path);
    }
    std::unordered_map<uint64_t, IndexEntry> index;
    uint64_t max_height = 0;
    std::unordered_map<uint64_t, uint64_t> max_height_in_blk;
    std::unordered_map<uint64_t, BLK> blks;
    DB db = DB::open(index_path, "default");
    db.new_iter();
    std::vector<uint8_t> key, value;
    while (true) {
        db.advance();
        db.current(key, value);
        if (is_block_index_entry(key)) {
            IndexEntry record = IndexEntry::from_leveldb_kv(std::vector<uint8_t>(key.begin() + 1, key.end()), value);
            if (record.status & (BLOCK_VALID_CHAIN | BLOCK_HAVE_DATA | BLOCK_VALID_CHAIN) > 0) {
                uint64_t& height_in_blk = max_height_in_blk[record.blk_index];
                if (record.height > height_in_blk) {
                    height_in_blk = record.height;
                }
                blks.emplace(record.blk_index, BLK(btc_data_dir, record.blk_index));
                if (record.height > max_height) {
                    max_height = record.height;
                }
                index.emplace(record.height, record);
            }
        }
    }
    for (const auto& entry : index) {
        if (entry.second.height != entry.first) {
            throw IndexError("Invalid height " + std::to_string(entry.second.height) + " in entry, expect: " + std::to_string(entry.first));
        }
    }
    std::cout << "All index entries are valid until height: " << max_height << "." << std::endl;
    return std::make_pair(index, max_height, max_height_in_blk, blks);
}

int main() {
    std::string btc_data_dir = "/path/to/btc_data_dir";
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::unordered_map<uint64_t, IndexEntry> entries;
    uint64_t max_height;
    std::unordered_map<uint64_t, uint64_t> max_height_in_blk;
    std::unordered_map<uint64_t, BLK> blks;
    try {
        std::tie(entries, max_height, max_height_in_blk, blks) = parse_index_for_ordinals(btc_data_dir);
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Parsed bitcoin index, " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s." << std::endl;
    } catch (const IndexError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}

