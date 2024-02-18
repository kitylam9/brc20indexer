

class Coin: Trait {
public:
    String name();
    // Magic value to identify blocks
    uint32 magic(Coin self);
    // https://en.bitcoin.it/wiki/List_of_address_prefixes
    uint8  version_id(Coin self);
    // Returns genesis hash
    fn genesis(&self) -> sha256d::Hash;
    // Activates AuxPow for the returned version and above
    fn aux_pow_activation_version(&self) -> Option<u32> {
        None
    }
    // Default working directory to look for datadir, for example .bitcoin
    fn default_folder(&self) -> PathBuf;
}