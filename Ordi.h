// EVMC: Ethereum Client-VM Connector API.
// Copyright 2016 The EVMC Authors.
// Licensed under the Apache License, Version 2.0.
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <bitcoincore_rpc.h>
#include <log.h>
#include <leveldb/db.h>  
#include <leveldb/write_batch.h> // leveldb::WriteBatch
#include <thiserror.h>
#include <vector>
#include <string> 
#include "bitcoin.h"
#include "block.h"
#include "epoch.h"
#include "height.h"
#include "inscription.h"
#include "client.h"
#include "db.h"
#include "index.h"
#include "updater.h"
#include "block_updater.h"
#include "ordi_error.h"
#include <evmc/evmc.h>
#include <evmc/helpers.h>
#include <evmc/instructions.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
const int FIRST_INSCRIPTION_HEIGHT = 0;

namespace fs = std::filesystem;

const std::string ORDI_STATUS = "status";
const std::string ORDI_OUTPUT_VALUE = "output_value";
const std::string ORDI_ID_TO_INSCRIPTION = "id_inscription";
const std::string ORDI_INSCRIPTION_TO_OUTPUT = "inscription_output";
const std::string ORDI_OUTPUT_TO_INSCRIPTION = "output_inscription";

class OrdiError : public std::exception {
public:
    OrdiError(const std::string& message) : message(message) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
private:
    std::string message;
};


class Options {
public:
    std::string btc_data_dir;
    std::string ordi_data_dir;
    std::string btc_rpc_host;
    std::string btc_rpc_user;
    std::string btc_rpc_pass;

    Options() :
        btc_data_dir(std::getenv("btc_data_dir") ? std::getenv("btc_data_dir") : ""),
        ordi_data_dir(std::getenv("ordi_data_dir") ? std::getenv("ordi_data_dir") : ""),
        btc_rpc_host(std::getenv("btc_rpc_host") ? std::getenv("btc_rpc_host") : ""),
        btc_rpc_user(std::getenv("btc_rpc_user") ? std::getenv("btc_rpc_user") : ""),
        btc_rpc_pass(std::getenv("btc_rpc_pass") ? std::getenv("btc_rpc_pass") : "") {}
};

class Ordi {
public:
    Client btc_rpc_client;
    DB status;
    DB output_value;
    DB id_inscription;
    DB inscription_output;
    DB output_inscription;
    Index index;
    std::vector<InscribeUpdater> inscribe_updaters;
    std::vector<TransferUpdater> transfer_updaters;

    void close() {
        status.close();
        output_value.close();
        id_inscription.close();
        inscription_output.close();
        output_inscription.close();
    }

    void start() {
        int next_height = index.max_height + 1;
        for (int height = FIRST_INSCRIPTION_HEIGHT; height < next_height; height++) {
            Block block = index.catch_block(height);
            BlockUpdater block_updater(height, block, btc_rpc_client, status, output_value, id_inscription, inscription_output, output_inscription, inscribe_updaters, transfer_updaters);
            block_updater.index_transactions();
        }
        while (true) {
            try {
                std::string block_hash = btc_rpc_client.get_block_hash(next_height);
                BlockUpdater block_updater(next_height, btc_rpc_client.get_block(block_hash), btc_rpc_client, status, output_value, id_inscription, inscription_output, output_inscription, inscribe_updaters, transfer_updaters);
                block_updater.index_transactions();
                next_height++;
            } catch (...) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
        }
    }

    void index_output_value() {
        for (int height = 0; height < FIRST_INSCRIPTION_HEIGHT; height++) {
            Block block = index.catch_block(height);
            for (int tx_index = 0; tx_index < block.txs.size(); tx_index++) {
                index_output_value_in_transaction(block.txs[tx_index]);
            }
        }
    }

    Ordi(const Options& options) {
        fs::path ordi_data_dir(options.ordi_data_dir);
        if (!fs::exists(ordi_data_dir)) {
            fs::create_directory(ordi_data_dir);
        }

        index = Index(fs::path(options.btc_data_dir));

        leveldb::Options leveldb_options;
        leveldb_options.max_file_size = 2 << 25;

        status = rusty_leveldb::DB::open(ordi_data_dir / ORDI_STATUS, leveldb_options);
        output_value = rusty_leveldb::DB::open(ordi_data_dir / ORDI_OUTPUT_VALUE, leveldb_options);
        id_inscription = rusty_leveldb::DB::open(ordi_data_dir / ORDI_ID_TO_INSCRIPTION, leveldb_options);
        inscription_output = rusty_leveldb::DB::open(ordi_data_dir / ORDI_INSCRIPTION_TO_OUTPUT, leveldb_options);
        output_inscription = rusty_leveldb::DB::open(ordi_data_dir / ORDI_OUTPUT_TO_INSCRIPTION, rusty_leveldb::in_memory());

        btc_rpc_client = bitcoincore_rpc::Client(options.btc_rpc_host, bitcoincore_rpc::Auth::UserPass(options.btc_rpc_user, options.btc_rpc_pass));
    }
    void when_inscribe(InscribeUpdater f) {
        inscribe_updaters.push_back(f);
    }

    void when_transfer(TransferUpdater f) {
        transfer_updaters.push_back(f);
    }
    
    void index_output_value_in_transaction(Tx tx) {
        WriteBatch wb;
        for (int output_index = 0; output_index < tx.value.outputs.size(); output_index++) {
            std::string k = tx.hash.to_string() + ":" + std::to_string(output_index);
            wb.put(k.c_str(), reinterpret_cast<const char*>(&tx.value.outputs[output_index].out.value));
        }

        for (Input input : tx.value.inputs) {
            if (input.outpoint.is_null()) {
                continue;
            }
            std::string k = input.outpoint.txid.to_string() + ":" + std::to_string(input.outpoint.index);
            wb.delete(k.c_str());
        }

        output_value.write(wb, false);
    }
};