#pragma once

#include <exception>
#include <string>

namespace stegtool {

class StegtoolException : public std::exception {
protected:
    std::string message_;

public:
    explicit StegtoolException(const std::string& msg) : message_(msg) {}
    const char* what() const noexcept override { return message_.c_str(); }
};

class CarrierException : public StegtoolException {
public:
    explicit CarrierException(const std::string& msg) : StegtoolException("Carrier error: " + msg) {}
};

class InvalidCarrierException : public CarrierException {
public:
    explicit InvalidCarrierException(const std::string& msg) : CarrierException("Invalid carrier: " + msg) {}
};

class InsufficientCapacityException : public CarrierException {
public:
    explicit InsufficientCapacityException(size_t required, size_t available)
        : CarrierException("Insufficient capacity: required " + std::to_string(required) + 
                           " bytes, available " + std::to_string(available)) {}
};

class EncryptionException : public StegtoolException {
public:
    explicit EncryptionException(const std::string& msg) : StegtoolException("Encryption error: " + msg) {}
};

class CompressionException : public StegtoolException {
public:
    explicit CompressionException(const std::string& msg) : StegtoolException("Compression error: " + msg) {}
};

class PayloadException : public StegtoolException {
public:
    explicit PayloadException(const std::string& msg) : StegtoolException("Payload error: " + msg) {}
};

class FileIOException : public StegtoolException {
public:
    explicit FileIOException(const std::string& msg) : StegtoolException("File I/O error: " + msg) {}
};

class CLIException : public StegtoolException {
public:
    explicit CLIException(const std::string& msg) : StegtoolException("CLI error: " + msg) {}
};

} // namespace stegtool
