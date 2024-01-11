#include <bitcoin/constants.hpp>
#include <derive_more/display.hpp>
#include "height.hpp"

const uint64_t COIN_VALUE = bitcoin::constants::COIN_VALUE;

const uint64_t SUBSIDY_HALVING_INTERVAL = static_cast<uint64_t>(bitcoin::blockdata::constants::SUBSIDY_HALVING_INTERVAL);

struct Epoch {
    uint64_t value;

    Epoch(uint64_t val) : value(val) {}

    static const Epoch FIRST_POST_SUBSIDY;

    uint64_t subsidy() {
        if (value < FIRST_POST_SUBSIDY.value) {
            return (50 * COIN_VALUE) >> value;
        } else {
            return 0;
        }
    }
};

const Epoch Epoch::FIRST_POST_SUBSIDY(33);

Epoch height_to_epoch(Height height) {
    return Epoch(height.value / SUBSIDY_HALVING_INTERVAL);
}