#include "cli.hpp"
#include "../core/exceptions.hpp"
#include "../utils/file_io.hpp"
#include "../carriers/carrier_factory.hpp"
#include "../payload/serializer.hpp"
#include "../payload/packet.hpp"
#include "../crypto/encryptor_factory.hpp"
#include "../core/encryptor.hpp"
#include <iostream>
#include <string>
#include <filesystem>

namespace stegtool::cli {

CLI::CLI(int argc, char* argv[]) : argc_(argc), argv_(argv) {}

static std::string arg_value(int argc, char* argv[], const std::string& key, const std::string& default_val = "") {
    for (int i = 2; i < argc; ++i) {
        if (argv[i] == key && i + 1 < argc) return argv[i + 1];
    }
    return default_val;
}

int CLI::run() {
    try {
        if (argc_ < 2) {
            print_help();
            return 1;
        }

        std::string command = argv_[1];

        if (command == "help" || command == "--help" || command == "-h") {
            print_help();
            return 0;
        } else if (command == "version" || command == "--version" || command == "-v") {
            print_version();
            return 0;
        } else if (command == "embed") {
            handle_embed();
            return 0;
        } else if (command == "extract") {
            handle_extract();
            return 0;
        } else {
            std::cerr << "Unknown command: " << command << std::endl;
            print_help();
            return 1;
        }
    } catch (const StegtoolException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

void CLI::print_help() const {
    std::cout << "stegtool - Terminal-Based Steganography Tool v1.0.0\n"
              << "A modular steganography tool for embedding encrypted and compressed files\n\n"
              << "Usage: stegtool [COMMAND] [OPTIONS]\n\n"
              << "Commands:\n"
              << "  embed       Embed a secret file into a carrier\n"
              << "  extract     Extract a secret file from a carrier\n"
              << "  help        Show this help message\n"
              << "  version     Show version information\n\n"
              << "Embed Options:\n"
              << "  --carrier <id>   Carrier format (bmp|png|wav|text, default: bmp)\n"
              << "  --in <path>      Input carrier file path (cover image/audio)\n"
              << "  --out <path>     Output stego file path (carrier with embedded data)\n"
              << "  --payload <path> Payload file to embed\n"
              << "  --encrypt <id>   Encryption algorithm (none|aes-gcm|chacha20|xor, default: none)\n"
              << "  --passphrase <p> Passphrase for encryption (required if --encrypt specified)\n\n"
              << "Extract Options:\n"
              << "  --in <path>      Input stego file path (carrier with embedded data)\n"
              << "  --out <path>     Output extracted payload path\n"
              << "  --carrier <id>   Carrier format (required)\n"
              << "  --encrypt <id>   Encryption algorithm (default: auto-detect)\n"
              << "  --passphrase <p> Passphrase for decryption\n\n"
              << "Examples:\n"
              << "  # Embed unencrypted:\n"
              << "  stegtool embed --carrier bmp --in cover.bmp --out stego.bmp --payload secret.txt\n\n"
              << "  # Embed with AES-GCM encryption:\n"
              << "  stegtool embed --carrier bmp --in cover.bmp --out stego.bmp \\\n"
              << "    --payload secret.txt --encrypt aes-gcm --passphrase mypassword\n\n"
              << "  # Extract:\n"
              << "  stegtool extract --in stego.bmp --out recovered.txt --encrypt aes-gcm \\\n"
              << "    --passphrase mypassword\n"
              << std::endl;
}

void CLI::print_version() const {
    std::cout << "stegtool version 1.0.0\n"
              << "A modular steganography tool\n"
              << "Copyright (c) 2024. Licensed under MIT.\n"
              << std::endl;
}

void CLI::handle_embed() {
    std::string carrier_id = arg_value(argc_, argv_, "--carrier", "bmp");
    std::string in_path = arg_value(argc_, argv_, "--in");
    std::string out_path = arg_value(argc_, argv_, "--out");
    std::string payload_path = arg_value(argc_, argv_, "--payload");

    if (in_path.empty() || out_path.empty() || payload_path.empty()) {
        std::cerr << "Error: embed requires --in, --out and --payload\n";
        print_help();
        throw CLIException("Missing required arguments for embed command");
    }

    // Validate file paths
    if (!std::filesystem::exists(in_path)) {
        throw CLIException("Input carrier file not found: " + in_path);
    }
    if (!std::filesystem::exists(payload_path)) {
        throw CLIException("Payload file not found: " + payload_path);
    }

    std::cout << "stegtool: Embedding payload into carrier...\n"
              << "  Carrier: " << carrier_id << "\n"
              << "  Cover file: " << in_path << "\n"
              << "  Output file: " << out_path << "\n"
              << "  Payload: " << payload_path << "\n";

    // Read payload
    ByteArray payload = stegtool::utils::FileIO::read_file(std::filesystem::path(payload_path));
    std::cout << "  Payload size: " << payload.size() << " bytes\n";

    // Serialize with compression
    using namespace stegtool::payload;
    Packet pkt = Serializer::serialize_with_compression(payload, true);
    std::cout << "  Serialized size: " << pkt.serialize().size() << " bytes\n";

    // Optional encryption
    std::string encrypt_id = arg_value(argc_, argv_, "--encrypt", "none");
    std::string passphrase = arg_value(argc_, argv_, "--passphrase", "");

    if (encrypt_id != "none" && passphrase.empty()) {
        throw CLIException("Encryption algorithm specified but no passphrase provided");
    }

    ByteArray to_embed_bytes;
    Packet final_pkt;
    if (encrypt_id != "none") {
        std::cout << "  Encrypting with: " << encrypt_id << "\n";
        // create encryptor
        auto enc = stegtool::crypto::EncryptorFactory::instance().create(encrypt_id);
        EncryptedData ed = enc->encrypt(pkt.serialize(), passphrase);
        // Build enc_meta: alg\0 iv_len iv salt_len salt tag_len tag
        ByteArray meta;
        std::string alg = enc->algorithm();
        meta.insert(meta.end(), alg.begin(), alg.end());
        meta.push_back(0);
        auto push_blob = [&](const ByteArray& b){ meta.push_back(static_cast<uint8_t>(b.size())); meta.insert(meta.end(), b.begin(), b.end()); };
        push_blob(ed.iv);
        push_blob(ed.salt);
        push_blob(ed.auth_tag);
        final_pkt = Packet(ed.ciphertext);
        final_pkt.set_encryption_metadata(meta);
        to_embed_bytes = final_pkt.serialize();
    } else {
        to_embed_bytes = pkt.serialize();
    }

    // Create carrier via factory using input cover file
    CarrierPtr carrier = carriers::CarrierFactory::instance().create(carrier_id, std::filesystem::path(in_path));
    
    std::cout << "  Carrier capacity: " << carrier->capacity() << " bytes\n";
    if (to_embed_bytes.size() > carrier->capacity()) {
        throw CarrierException("Payload too large for carrier: " + 
                              std::to_string(to_embed_bytes.size()) + " > " +
                              std::to_string(carrier->capacity()));
    }

    // Embed and save
    carrier->embed(to_embed_bytes);
    carrier->save(std::filesystem::path(out_path));

    std::cout << "✓ Embed completed successfully\n"
              << "  Output: " << out_path << "\n";
}

void CLI::handle_extract() {
    std::string carrier_id = arg_value(argc_, argv_, "--carrier", "bmp");
    std::string in_path = arg_value(argc_, argv_, "--in");
    std::string out_path = arg_value(argc_, argv_, "--out");
    std::string encrypt_id = arg_value(argc_, argv_, "--encrypt", "none");
    std::string passphrase = arg_value(argc_, argv_, "--passphrase", "");

    if (in_path.empty() || out_path.empty()) {
        std::cerr << "Error: extract requires --in and --out\n";
        print_help();
        throw CLIException("Missing required arguments for extract command");
    }

    if (!std::filesystem::exists(in_path)) {
        throw CLIException("Input stego file not found: " + in_path);
    }

    std::cout << "stegtool: Extracting payload from carrier...\n"
              << "  Carrier: " << carrier_id << "\n"
              << "  Input file: " << in_path << "\n"
              << "  Output file: " << out_path << "\n";

    CarrierPtr carrier = carriers::CarrierFactory::instance().create(carrier_id, std::filesystem::path(in_path));
    ByteArray extracted = carrier->extract();
    std::cout << "  Extracted " << extracted.size() << " bytes\n";

    // Try to deserialize packet from extracted bytes
    using namespace stegtool::payload;
    try {
        Packet pkt = Packet::deserialize(extracted);
        ByteArray payload_bytes = pkt.data();
        // if packet has encryption metadata, decrypt
        ByteArray enc_meta = pkt.encryption_metadata();
        if (!enc_meta.empty()) {
            std::cout << "  Decrypting payload...\n";
            // parse meta: alg\0 ivlen iv saltlen salt taglen tag
            size_t pos = 0;
            std::string alg;
            while (pos < enc_meta.size() && enc_meta[pos] != 0) alg.push_back(static_cast<char>(enc_meta[pos++]));
            if (pos < enc_meta.size() && enc_meta[pos]==0) ++pos;
            auto read_blob = [&](ByteArray &out)->bool{
                if (pos >= enc_meta.size()) return false;
                uint8_t len = enc_meta[pos++];
                if (pos + len > enc_meta.size()) return false;
                out.insert(out.end(), enc_meta.begin() + pos, enc_meta.begin() + pos + len);
                pos += len;
                return true;
            };
            ByteArray iv, salt, tag;
            read_blob(iv); read_blob(salt); read_blob(tag);

            // use factory to create encryptor by alg (map alg names)
            std::string enc_id = encrypt_id;
            if (enc_id == "none") {
                // try to map alg
                if (alg.find("AES") != std::string::npos) enc_id = "aes-gcm";
                else if (alg.find("ChaCha") != std::string::npos) enc_id = "chacha20";
                else if (alg == "XOR-Simple") enc_id = "xor";
            }
            
            if (passphrase.empty()) {
                throw CLIException("Payload is encrypted but no passphrase provided");
            }
            
            auto enc = stegtool::crypto::EncryptorFactory::instance().create(enc_id);
            EncryptedData ed;
            ed.ciphertext = payload_bytes;
            ed.iv = iv; ed.salt = salt; ed.auth_tag = tag; ed.algorithm = alg;
            ByteArray decrypted = enc->decrypt(ed, passphrase);
            // deserialized inner packet
            Packet inner = Packet::deserialize(decrypted);
            ByteArray outdata = inner.data();
            stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), outdata);
            std::cout << "  ✓ Decryption successful\n";
        } else {
            // no encryption metadata: payload_bytes may be packet content or raw
            // attempt to deserialize inner packet
            try {
                Packet inner = Packet::deserialize(payload_bytes);
                stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), inner.data());
            } catch (...) {
                // fallback: write raw payload_bytes
                stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), payload_bytes);
            }
        }
        std::cout << "✓ Extract completed successfully\n"
                  << "  Output: " << out_path << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to deserialize packet (" << e.what() << ")\n";
        std::cout << "  Writing raw extracted data...\n";
        stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), extracted);
        std::cout << "✓ Extract completed (raw data)\n"
                  << "  Output: " << out_path << "\n";
    }
}

} // namespace stegtool::cli
