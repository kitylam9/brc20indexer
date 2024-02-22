#include "crypto.h"

class Coin: Trait {
public:
    String name();
    // Magic value to identify blocks
    uint32 magic(Coin self);
    // https://en.bitcoin.it/wiki/List_of_address_prefixes
    uint8  version_id(Coin self);
    // Returns genesis hash
    sha256d::Hash genesis(Coin self)  ;
    // Activates AuxPow for the returned version and above
   Option<uint32> aux_pow_activation_version(Coin self) -> Option<uint32> {
        None
    }
    // Default working directory to look for datadir, for example .bitcoin
    PathBuf default_folder(Coin self) ;
}