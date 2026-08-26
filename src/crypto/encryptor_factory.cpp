#include "encryptor_factory.hpp"
#include "../core/exceptions.hpp"
#include "aes_gcm.hpp"
#include "chacha20_poly1305.hpp"
#include "xor_encryptor.hpp"
#include "rsa_hybrid.hpp"
#include "noop_encryptor.hpp"

namespace stegtool::crypto {

EncryptorFactory& EncryptorFactory::instance() {
    static EncryptorFactory factory;
    return factory;
}

EncryptorFactory::EncryptorFactory() {
    register_creator("none", [](){ return std::make_unique<NoopEncryptor>(); });
#ifdef HAVE_OPENSSL
    register_creator("aes-gcm", [](){ return std::make_unique<AESGCM>(); });
    register_creator("chacha20", [](){ return std::make_unique<ChaCha20Poly1305>(); });
    register_creator("xor", [](){ return std::make_unique<XOREncryptor>(); });
    register_creator("rsa-hybrid", [](){ return std::make_unique<RSAHybrid>(); });
#endif
}

void EncryptorFactory::register_creator(const std::string& id, Creator creator) {
    creators_.emplace(id, std::move(creator));
}

stegtool::EncryptorPtr EncryptorFactory::create(const std::string& id) const {
    auto it = creators_.find(id);
    if (it == creators_.end()) {
        throw stegtool::EncryptionException("Unknown encryptor id: " + id);
    }
    return it->second();
}

std::vector<std::string> EncryptorFactory::available() const {
    std::vector<std::string> keys;
    keys.reserve(creators_.size());
    for (auto &p : creators_) keys.push_back(p.first);
    return keys;
}

} // namespace stegtool::crypto
