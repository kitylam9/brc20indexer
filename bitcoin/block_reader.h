#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <stdexcept>

namespace anyhow {
    class Error : public std::exception {
    public:
        Error(const std::string& message) : message(message) {}
        const char* what() const noexcept override {
            return message.c_str();
        }
    private:
        std::string message;
    };

    template<typename T>
    class Result {
    public:
        Result(const T& value) : value(value), hasValue(true) {}
        Result(const Error& error) : error(error), hasValue(false) {}
        bool isOk() const {
            return hasValue;
        }
        bool isErr() const {
            return !hasValue;
        }
        const T& unwrap() const {
            if (hasValue) {
                return value;
            } else {
                throw std::runtime_error(error.what());
            }
        }
    private:
        T value;
        Error error;
        bool hasValue;
    };
}

namespace sha256d {
    class Hash {
    public:
        Hash(const std::array<uint8_t, 32>& data) : data(data) {}
        static Hash fromByteArray(const std::array<uint8_t, 32>& data) {
            return Hash(data);
        }
    private:
        std::array<uint8_t, 32> data;
    };
}

class VarUint {
public:
    VarUint(uint64_t value) : value(value) {}
    static VarUint readFrom(BlockchainRead& reader) {
        // implementation
    }
private:
    uint64_t value;
};

class CoinType {
public:
    CoinType(uint32_t auxPowActivationVersion, uint8_t versionId) : auxPowActivationVersion(auxPowActivationVersion), versionId(versionId) {}
    uint32_t auxPowActivationVersion;
    uint8_t versionId;
};

class BlockHeader {
public:
    BlockHeader(uint32_t version, const sha256d::Hash& prevHash, const sha256d::Hash& merkleRoot, uint32_t timestamp, uint32_t bits, uint32_t nonce) : version(version), prevHash(prevHash), merkleRoot(merkleRoot), timestamp(timestamp), bits(bits), nonce(nonce) {}
    uint32_t version;
    sha256d::Hash prevHash;
    sha256d::Hash merkleRoot;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
};

class Block {
public:
    Block(uint32_t size, const BlockHeader& header, const AuxPowExtension& auxPowExtension, const VarUint& txCount, const std::vector<RawTx>& txs) : size(size), header(header), auxPowExtension(auxPowExtension), txCount(txCount), txs(txs) {}
    uint32_t size;
    BlockHeader header;
    AuxPowExtension auxPowExtension;
    VarUint txCount;
    std::vector<RawTx> txs;
};

class RawTx {
public:
    RawTx(uint32_t version, const VarUint& inCount, const std::vector<TxInput>& inputs, const VarUint& outCount, const std::vector<TxOutput>& outputs, uint32_t locktime, uint8_t versionId) : version(version), inCount(inCount), inputs(inputs), outCount(outCount), outputs(outputs), locktime(locktime), versionId(versionId) {}
    uint32_t version;
    VarUint inCount;
    std::vector<TxInput> inputs;
    VarUint outCount;
    std::vector<TxOutput> outputs;
    uint32_t locktime;
    uint8_t versionId;
};

class TxOutpoint {
public:
    TxOutpoint(const sha256d::Hash& txid, uint32_t index) : txid(txid), index(index) {}
    sha256d::Hash txid;
    uint32_t index;
};

class TxInput {
public:
    TxInput(const TxOutpoint& outpoint, const VarUint& scriptLen, const std::vector<uint8_t>& scriptSig, uint32_t seqNo, const std::optional<Witness>& witness) : outpoint(outpoint), scriptLen(scriptLen), scriptSig(scriptSig), seqNo(seqNo), witness(witness) {}
    TxOutpoint outpoint;
    VarUint scriptLen;
    std::vector<uint8_t> scriptSig;
    uint32_t seqNo;
    std::optional<Witness> witness;
};

class TxOutput {
public:
    TxOutput(uint64_t value, const VarUint& scriptLen, const std::vector<uint8_t>& scriptPubkey) : value(value), scriptLen(scriptLen), scriptPubkey(scriptPubkey) {}
    uint64_t value;
    VarUint scriptLen;
    std::vector<uint8_t> scriptPubkey;
};

class MerkleBranch {
public:
    MerkleBranch(const std::vector<std::array<uint8_t, 32>>& hashes, uint32_t sideMask) : hashes(hashes), sideMask(sideMask) {}
    std::vector<std::array<uint8_t, 32>> hashes;
    uint32_t sideMask;
};

class AuxPowExtension {
public:
    AuxPowExtension(const RawTx& coinbaseTx, const sha256d::Hash& blockHash, const MerkleBranch& coinbaseBranch, const MerkleBranch& blockchainBranch, const BlockHeader& parentBlock) : coinbaseTx(coinbaseTx), blockHash(blockHash), coinbaseBranch(coinbaseBranch), blockchainBranch(blockchainBranch), parentBlock(parentBlock) {}
    RawTx coinbaseTx;
    sha256d::Hash blockHash;
    MerkleBranch coinbaseBranch;
    MerkleBranch blockchainBranch;
    BlockHeader parentBlock;
};

class BlockchainRead {
public:
    virtual ~BlockchainRead() {}
    virtual std::array<uint8_t, 32> read256Hash() = 0;
    virtual std::vector<uint8_t> readU8Vec(uint32_t count) = 0;
    virtual Block readBlock(uint32_t size, const CoinType& coin) = 0;
    virtual BlockHeader readBlockHeader() = 0;
    virtual std::vector<RawTx> readTxs(uint64_t txCount, uint8_t versionId) = 0;
    virtual RawTx readTx(uint8_t versionId) = 0;
    virtual TxOutpoint readTxOutpoint() = 0;
    virtual std::vector<TxInput> readTxInputs(uint64_t inputCount) = 0;
    virtual std::vector<TxOutput> readTxOutputs(uint64_t outputCount) = 0;
    virtual MerkleBranch readMerkleBranch() = 0;
    virtual AuxPowExtension readAuxPowExtension(uint8_t versionId) = 0;
};

template<typename R>
class BlockchainReadImpl : public BlockchainRead {
public:
    BlockchainReadImpl(R& reader) : reader(reader) {}
    std::array<uint8_t, 32> read256Hash() override {
        std::array<uint8_t, 32> arr;
        reader.readExact(arr.data());
        return arr;
    }
    std::vector<uint8_t> readU8Vec(uint32_t count) override {
        std::vector<uint8_t> arr(count);
        reader.readExact(arr.data());
        return arr;
    }
    Block readBlock(uint32_t size, const CoinType& coin) override {
        BlockHeader header = readBlockHeader();
        std::optional<AuxPowExtension> auxPowExtension;
        if (coin.auxPowActivationVersion && header.version >= *coin.auxPowActivationVersion) {
            auxPowExtension = readAuxPowExtension(coin.versionId);
        }
        VarUint txCount = VarUint::readFrom(reader);
        std::vector<RawTx> txs = readTxs(txCount.unwrap().value, coin.versionId);
        return Block(size, header, auxPowExtension, txCount, txs);
    }
    BlockHeader readBlockHeader() override {
        uint32_t version = reader.readU32();
        sha256d::Hash prevHash = sha256d::Hash::fromByteArray(read256Hash());
        sha256d::Hash merkleRoot = sha256d::Hash::fromByteArray(read256Hash());
        uint32_t timestamp = reader.readU32();
        uint32_t bits = reader.readU32();
        uint32_t nonce = reader.readU32();
        return BlockHeader(version, prevHash, merkleRoot, timestamp, bits, nonce);
    }
    std::vector<RawTx> readTxs(uint64_t txCount, uint8_t versionId) override {
        std::vector<RawTx> txs;
        for (uint64_t i = 0; i < txCount; i++) {
            txs.push_back(readTx(versionId));
        }
        return txs;
    }
    RawTx readTx(uint8_t versionId) override {
        uint8_t flags = 0;
        uint32_t version = reader.readU32();
        VarUint inCount = VarUint::readFrom(reader);
        if (inCount.unwrap().value == 0) {
            flags = reader.readU8();
            inCount = VarUint::readFrom(reader);
        }
        std::vector<TxInput> inputs = readTxInputs(inCount.unwrap().value);
        VarUint outCount = VarUint::readFrom(reader);
        std::vector<TxOutput> outputs = readTxOutputs(outCount.unwrap().value);
        if (flags & 1) {
            for (uint64_t witnessIndex = 0; witnessIndex < inCount.unwrap().value; witnessIndex++) {
                VarUint itemCount = VarUint::readFrom(reader);
                std::vector<std::vector<uint8_t>> witnesses;
                for (uint64_t j = 0; j < itemCount.unwrap().value; j++) {
                    VarUint witnessLen = VarUint::readFrom(reader);
                    std::vector<uint8_t> witness = readU8Vec(witnessLen.unwrap().value);
                    witnesses.push_back(witness);
                }
                inputs[witnessIndex].witness = Witness::fromSlice(witnesses);
            }
        }
        uint32_t locktime = reader.readU32();
        return RawTx(version, inCount, inputs, outCount, outputs, locktime, versionId);
    }
    TxOutpoint readTxOutpoint() override {
        sha256d::Hash txid = sha256d::Hash::fromByteArray(read256Hash());
        uint32_t index = reader.readU32();
        return TxOutpoint(txid, index);
    }
    std::vector<TxInput> readTxInputs(uint64_t inputCount) override {
        std::vector<TxInput> inputs;
        for (uint64_t i = 0; i < inputCount; i++) {
            TxOutpoint outpoint = readTxOutpoint();
            VarUint scriptLen = VarUint::readFrom(reader);
            std::vector<uint8_t> scriptSig = readU8Vec(scriptLen.unwrap().value);
            uint32_t seqNo = reader.readU32();
            inputs.push_back(TxInput(outpoint, scriptLen, scriptSig, seqNo, std::nullopt));
        }
        return inputs;
    }
    std::vector<TxOutput> readTxOutputs(uint64_t outputCount) override {
        std::vector<TxOutput> outputs;
        for (uint64_t i = 0; i < outputCount; i++) {
            uint64_t value = reader.readU64();
            VarUint scriptLen = VarUint::readFrom(reader);
            std::vector<uint8_t> scriptPubkey = readU8Vec(scriptLen.unwrap().value);
            outputs.push_back(TxOutput(value, scriptLen, scriptPubkey));
        }
        return outputs;
    }
    MerkleBranch readMerkleBranch() override {
        VarUint branchLength = VarUint::readFrom(reader);
        std::vector<std::array<uint8_t, 32>> hashes;
        for (uint64_t i = 0; i < branchLength.unwrap().value; i++) {
            hashes.push_back(read256Hash());
        }
        uint32_t sideMask = reader.readU32();
        return MerkleBranch(hashes, sideMask);
    }
    AuxPowExtension readAuxPowExtension(uint8_t versionId) override {
        RawTx coinbaseTx = readTx(versionId);
        sha256d::Hash blockHash = sha256d::Hash::fromByteArray(read256Hash());
        MerkleBranch coinbaseBranch = readMerkleBranch();
        MerkleBranch blockchainBranch = readMerkleBranch();
        BlockHeader parentBlock = readBlockHeader();
        return AuxPowExtension(coinbaseTx, blockHash, coinbaseBranch, blockchainBranch, parentBlock);
    }
private:
    R& reader;
};

template<typename R>
BlockchainReadImpl<R> makeBlockchainRead(R& reader) {
    return BlockchainReadImpl<R>(reader);
}

class FileReader : public io::Read {
public:
    FileReader(const std::string& filename) {
        // implementation
    }
    void readExact(uint8_t* buffer) {
        // implementation
    }
    uint32_t readU32() {
        // implementation
    }
    uint8_t readU8() {
        // implementation
    }
};

class Witness {
public:
    static Witness fromSlice(const std::vector<std::vector<uint8_t>>& slices) {
        // implementation
    }
};

int main() {
    // implementation
    return 0;
}


