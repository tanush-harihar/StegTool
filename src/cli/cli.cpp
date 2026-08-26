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
    std::cout << "stegtool - Terminal-Based Steganography Tool\n"
              << "Usage: stegtool [COMMAND] [OPTIONS]\n\n"
              << "Commands:\n"
              << "  embed       Embed a secret file into a carrier\n"
              << "  extract     Extract a secret file from a carrier\n"
              << "  help        Show this help message\n"
              << "  version     Show version information\n\n"
              << "Options (embed):\n"
              << "  --carrier <id>   Carrier id (bmp|png|wav|text)\n"
              << "  --in <path>      Input carrier file path (cover image)\n"
              << "  --out <path>     Output carrier file path (stego image)\n"
              << "  --payload <path> Payload file to embed\n"
              << "Options (extract):\n"
              << "  --in <path>      Input steg carrier file path\n"
              << "  --out <path>     Output extracted payload path\n"
              << std::endl;
}

void CLI::print_version() const {
    std::cout << "stegtool version 1.0.0\n"
              << "A modular steganography tool\n"
              << std::endl;
}

void CLI::handle_embed() {
    std::string carrier_id = arg_value(argc_, argv_, "--carrier", "bmp");
    std::string in_path = arg_value(argc_, argv_, "--in");
    std::string out_path = arg_value(argc_, argv_, "--out");
    std::string payload_path = arg_value(argc_, argv_, "--payload");

    if (in_path.empty() || out_path.empty() || payload_path.empty()) {
        throw CLIException("embed requires --in, --out and --payload");
    }

    // Read payload
    ByteArray payload = stegtool::utils::FileIO::read_file(std::filesystem::path(payload_path));

    // Serialize with compression
    using namespace stegtool::payload;
    Packet pkt = Serializer::serialize_with_compression(payload, true);

    // Optional encryption
    std::string encrypt_id = arg_value(argc_, argv_, "--encrypt", "none");
    std::string passphrase = arg_value(argc_, argv_, "--passphrase", "");

    ByteArray to_embed_bytes;
    Packet final_pkt;
    if (encrypt_id != "none") {
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

    // Embed and save
    carrier->embed(to_embed_bytes);
    carrier->save(std::filesystem::path(out_path));

    std::cout << "Embed completed: " << out_path << std::endl;
}

void CLI::handle_extract() {
    std::string carrier_id = arg_value(argc_, argv_, "--carrier", "bmp");
    std::string in_path = arg_value(argc_, argv_, "--in");
    std::string out_path = arg_value(argc_, argv_, "--out");
    std::string encrypt_id = arg_value(argc_, argv_, "--encrypt", "none");
    std::string passphrase = arg_value(argc_, argv_, "--passphrase", "");

    if (in_path.empty() || out_path.empty()) {
        throw CLIException("extract requires --in and --out");
    }

    CarrierPtr carrier = carriers::CarrierFactory::instance().create(carrier_id, std::filesystem::path(in_path));
    ByteArray extracted = carrier->extract();

    // Try to deserialize packet from extracted bytes
    using namespace stegtool::payload;
    try {
        Packet pkt = Packet::deserialize(extracted);
        ByteArray payload_bytes = pkt.data();
        // if packet has encryption metadata, decrypt
        ByteArray enc_meta = pkt.encryption_metadata();
        if (!enc_meta.empty()) {
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
            auto enc = stegtool::crypto::EncryptorFactory::instance().create(enc_id);
            EncryptedData ed;
            ed.ciphertext = payload_bytes;
            ed.iv = iv; ed.salt = salt; ed.auth_tag = tag; ed.algorithm = alg;
            ByteArray decrypted = enc->decrypt(ed, passphrase);
            // deserialized inner packet
            Packet inner = Packet::deserialize(decrypted);
            ByteArray outdata = inner.data();
            stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), outdata);
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
    } catch (const std::exception&) {
        // fallback: write raw extracted bytes
        stegtool::utils::FileIO::write_file(std::filesystem::path(out_path), extracted);
    }

    std::cout << "Extract completed: " << out_path << std::endl;
}

} // namespace stegtool::cli
