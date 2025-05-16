#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <curl/curl.h>

#include "bip39-words.hpp"
#include "addres_generator.h"
#include "address_checker.h"

static const std::string save_found_path = "found_wallets.txt";
static std::string walletgen_path;

// ANSI color codes for Unix terminals
void set_color(int color = 7) {
    switch (color) {
        case 4: std::cout << "\033[1;31m"; break; // Red
        case 6: std::cout << "\033[1;33m"; break; // Yellow
        case 7: std::cout << "\033[0m";    break; // Reset
        case 8: std::cout << "\033[1;36m"; break; // Cyan
        case 9: std::cout << "\033[1;34m"; break; // Blue
        default: std::cout << "\033[0m";   break;
    }
}

void print_logo() {
    std::cout << "***************************************************************************\n";
    std::cout << "*__        __    _ _      _    ____                                       *\n";
    std::cout << "*\\ \\      / /_ _| | | ___| |_ / ___| ___ _ __                             *\n";
    std::cout << "* \\ \\ /\\ / / _` | | |/ _ \\ __| |  _ / _ \\ '_ \\                            *\n";
    std::cout << "*  \\ V  V / (_| | | |  __/ |_| |_| |  __/ | | |                           *\n";
    std::cout << "*   \\_/\\_/ \\__,_|_|_|\\___|\\__|\\____|\\___|_| |_|             ____  _       *\n";
    std::cout << "*                  | |__  _   _  |_   _|__  _ __  _   _    | __ )| |_ ___ *\n";
    std::cout << "*                  | '_ \\| | | |   | |/ _ \\| '_ \\| | | |   |  _ \\| __/ __|*\n";
    std::cout << "*                  | |_) | |_| |   | | (_) | | | | |_| |   | |_) | || (__ *\n";
    std::cout << "*                  |_.__/ \\__, |   |_|\\___/|_| |_|\\__, |___|____/ \\__\\___|*\n";
    std::cout << "*                         |___/                   |___/_____|             *\n";
    std::cout << "***************************************************************************\n";
    std::cout << "Github: https://github.com/tony-dev-btc/walletgen\n";
    std::cout << "\n";
}

void print_donate_info() {
    std::cout << "---------------------------------------------------------------------------------\n";
    std::cout << "| my crypto wallets for donate:                                                 |\n";
    std::cout << "| Bitcoin:                       bc1qm2hla5zg735uqpgjtp3va4djqhh9zd8derzrwx     |\n";
    std::cout << "| EVM (ETH, BNB, MATIC, e.t.c):  0x8BCa04cA3e65fa7ECE2314e02149F6Ef4F3A0060     |\n";
    std::cout << "| Litecoin:                      ltc1qa4ermet8czres4aunf33zwe35sk6peq6tg6cc8    |\n";
    std::cout << "| USDT TRC20:                    TFu3ydp1tSDhCtrHNsVSVxriVGhHvVzUu9             |\n";
    std::cout << "---------------------------------------------------------------------------------\n";
}

int show_menu() {
    set_color(9);
    std::cout << "============================================================================\n";
    std::cout << "'1' - generate one BTC wallet                                              =\n";
    std::cout << "'2' - generate one EVM wallet (ETH, BNB, MATIC e.t.c)                      =\n";
    std::cout << "'3' - search BTC wallets with balance (using the Internet - slower)        =\n";
    std::cout << "'4' - search BTC wallets with balance (using the database - faster)        =\n";
    std::cout << "'5' - search EVM wallets with balance (using the Internet - slower)        =\n";
    std::cout << "'6' - search EVM wallets with balance (using the database - faster)        =\n";
    std::cout << "'7' - support the developer                                                =\n";
    std::cout << "'8' - exit                                                                 =\n";
    std::cout << "============================================================================\n";
    set_color(7);

    std::cout << "select an action using the key: ";
    int action;
    std::cin >> action;
    return action;
}

void print_wallet(const wallet& wal, int type) {
    set_color(8);
    std::cout << "-----------------------------------------------------------------------------\n";
    std::cout << (type == WAL_TYPE_BTC ? "BTC" : "EVM") << " address: " << wal.address << "\n";
    std::cout << "public key:  " << wal.public_key << "\n";
    std::cout << "private key: " << wal.private_key << "\n";
    std::cout << "mnemonic:    " << wal.mnemonic << "\n";
    std::cout << "-----------------------------------------------------------------------------\n\n";
    set_color(7);
}

void save_wallet_data(const wallet& wal, const wallet_data& wal_data, int type) {
    std::ofstream outfile(save_found_path, std::ios::app);
    if (!outfile) {
        std::cerr << "Failed to open " << save_found_path << " for writing.\n";
        return;
    }
    if (type == WAL_TYPE_BTC) {
        outfile << "[Bitcoin]\n"
                << "address: " << wal.address << "\n"
                << "public key: " << wal.public_key << "\n"
                << "private key: " << wal.private_key << "\n"
                << "mnemonic: " << wal.mnemonic << "\n"
                << "btc_amount: " << wal_data.btc_amount << "\n\n";
    } else {
        outfile << "[EVM]\n"
                << "address: " << wal.address << "\n"
                << "public key: " << wal.public_key << "\n"
                << "private key: " << wal.private_key << "\n"
                << "mnemonic: " << wal.mnemonic << "\n"
                << "eth_amount: " << wal_data.eth_amount << "\n"
                << "bnb_amount: " << wal_data.bnb_amount << "\n"
                << "matic_amount: " << wal_data.matic_amount << "\n\n";
    }
}

void start_search_btc_wallets(int way = 0);
void start_search_evm_wallets(int way = 0);

int main(int argc, char** argv) {
    // Determine executable path
    walletgen_path = std::string(argv[0]);
    size_t pos = walletgen_path.find_last_of('/');
    if (pos != std::string::npos) walletgen_path = walletgen_path.substr(0, pos);

    if (!load_bip39_words()) {
        std::cerr << "Failed to load BIP39 words from 'bip39-words.txt'.\n";
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        if (std::string(argv[1]) == "gen-btc") {
            int count = (argc > 2) ? std::atoi(argv[2]) : 1;
            for (int i = 0; i < count; ++i) {
                std::string mn = generate_mnemonic(12);
                wallet w = generate_bitcoin_wallet(mn);
                print_wallet(w, WAL_TYPE_BTC);
            }
        } else if (std::string(argv[1]) == "gen-evm") {
            int count = (argc > 2) ? std::atoi(argv[2]) : 1;
            for (int i = 0; i < count; ++i) {
                std::string mn = generate_mnemonic(12);
                wallet w = generate_evm_wallet(mn);
                print_wallet(w, WAL_TYPE_EVM);
            }
        } else if (std::string(argv[1]) == "search-btc") {
            start_search_btc_wallets();
        } else if (std::string(argv[1]) == "search-evm") {
            start_search_evm_wallets();
        } else {
            std::cerr << "Invalid parameters.\n";
        }
        return EXIT_SUCCESS;
    }

    set_color(6);
    print_logo();
    set_color(7);

    while (true) {
        int action = show_menu();
        std::cout << "\n";
        switch (action) {
            case 1: {
                std::string mn = generate_mnemonic(12);
                print_wallet(generate_bitcoin_wallet(mn), WAL_TYPE_BTC);
                break;
            }
            case 2: {
                std::string mn = generate_mnemonic(12);
                print_wallet(generate_evm_wallet(mn), WAL_TYPE_EVM);
                break;
            }
            case 3: start_search_btc_wallets(0); break;
            case 4: start_search_btc_wallets(1); break;
            case 5: start_search_evm_wallets(0); break;
            case 6: start_search_evm_wallets(1); break;
            case 7: print_donate_info(); break;
            case 8: return EXIT_SUCCESS;
            default: std::cout << "Invalid action\n"; break;
        }
        std::cout << "\n\n";
    }
    return EXIT_SUCCESS;
}