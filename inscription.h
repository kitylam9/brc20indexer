#include <iostream>
#include <vector>
#include <array>
#include <iterator>
#include <algorithm> 
#include <optional>
#include <stdexcept>
#include <bitcoin/system.hpp>
#include <bitcoin/taproot.hpp>
#include "block.hpp"

enum class Curse {
    NotInFirstInput,
    NotAtOffsetZero,
    Reinscription
};

class InscriptionError : public std::runtime_error {
public:
    InscriptionError(const std::string& message) : std::runtime_error(message) {}
};

struct Inscription {
    std::optional<std::vector<uint8_t>> body;
    std::optional<std::vector<uint8_t>> content_type;
};

struct TransactionInscription {
    Inscription inscription;
    uint32_t tx_in_index;
    uint32_t tx_in_offset;
};

class InscriptionParser {
public:
    std::vector<std::result<Inscription>> parseInscriptions() {
        std::vector<std::result<Inscription>> inscriptions;
        while (true) {
            auto current = parseOneInscription();
            if (current == std::errc::no_inscription) {
                break;
            }
            inscriptions.push_back(current);
        }
        return inscriptions;
    }
    std::result<Inscription> parseOneInscription() {
        advanceIntoInscriptionEnvelope();

        std::map<std::string, std::vector<uint8_t>> fields;

        while (true) {
            auto nextInstruction = advance();
            if (nextInstruction == Instruction::PushBytes(BODY_TAG)) {
                std::vector<uint8_t> body;
                while (!accept(Instruction::Op(opcodes::all::OP_ENDIF))) {
                    auto pushResult = expectPush();
                    body.insert(body.end(), pushResult.begin(), pushResult.end());
                }
                fields[BODY_TAG] = body;
                break;
            } else if (nextInstruction == Instruction::PushBytes(tag)) {
                if (fields.find(tag) != fields.end()) {
                    return std::errc::invalid_inscription;
                }
                fields[tag] = expectPush();
            } else if (nextInstruction == Instruction::Op(opcodes::all::OP_ENDIF)) {
                break;
            } else {
                return std::errc::invalid_inscription;
            }
        }

        auto body = fields[BODY_TAG];
        auto contentType = fields[CONTENT_TYPE_TAG];
        
        for (auto const& tag : fields) {
            if (tag.first.front() % 2 == 0) {
                return std::errc::unrecognized_even_field;
            }
        }

        return Inscription{body, contentType};
    }

    std::result<Instruction> advance() {
        if (instructions.empty()) {
            return std::errc::no_inscription;
        }
        return instructions.front();
    }

    std::result<void> advanceIntoInscriptionEnvelope() {
        std::vector<Instruction> inscriptionEnvelopeHeader = {
            Instruction::PushBytes({}), // This is an OP_FALSE
            Instruction::Op(opcodes::all::OP_IF),
            Instruction::PushBytes(PROTOCOL_ID),
        };
        while (!matchInstructions(inscriptionEnvelopeHeader)) {
            // Continue looping until the header is matched
        }
        return std::errc::success;
    }

    std::result<bool> matchInstructions(std::vector<Instruction> instructions) {
        for (auto const& instruction : instructions) {
            if (advance() != instruction) {
                return false;
            }
        }
        return true;
    }

    std::result<std::vector<uint8_t>> expectPush() {
        auto nextInstruction = advance();
        if (nextInstruction == Instruction::PushBytes(bytes)) {
            return bytes;
        } else {
            return std::errc::invalid_inscription;
        }
    }

    std::result<bool> accept(Instruction instruction) {
        if (!instructions.empty()) {
            if (instructions.front() == instruction) {
                advance();
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

private:
    std::vector<Instruction> instructions;
    static std::vector<TransactionInscription> from_transaction(const Tx& tx) {
        std::vector<TransactionInscription> result;
        for (size_t index = 0; index < tx.inputs.size(); ++index) {
            const auto& tx_in = tx.inputs[index];
            if (!tx_in.witness.has_value()) {
                continue;
            }

            try {
                auto inscriptions = parse(tx_in.witness.value());
                for (size_t offset = 0; offset < inscriptions.size(); ++offset) {
                    result.push_back({inscriptions[offset], static_cast<uint32_t>(index), static_cast<uint32_t>(offset)});
                }
            } catch (const InscriptionError&) {
                continue;
            }
        }
        return result;
    }

private:
    static std::vector<Inscription> parse(const Witness& witness) {
        if (witness.empty()) {
            throw InscriptionError("empty witness");
        }

        if (witness.size() == 1) {
            throw InscriptionError("key-path spend");
        }

        bool annex = witness.back().front() == TAPROOT_ANNEX_PREFIX;

        if (witness.size() == 2 && annex) {
            throw InscriptionError("key-path spend");
        }

        const auto& script = annex ? witness[witness.size() - 1] : witness[witness.size() - 2];
        InscriptionParser parser{Script::from_bytes(script).instructions()};
        return parser.parse_inscriptions();
    }

    Peekable<Instructions> instructions;

    explicit InscriptionParser(Instructions instructions) : instructions(std::move(instructions)) {}

    std::vector<Inscription> parse_inscriptions() {
        std::vector<Inscription> inscriptions;
        while (instructions.peek() != instructions.end()) {
            // Parse inscriptions
        }
        return inscriptions;
    }
};