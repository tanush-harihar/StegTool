# stegtool - Terminal-Based Steganography Tool

A production-quality educational CLI application for hiding encrypted and compressed binary files inside different carrier formats.

## Features

- **Modular Architecture**: Clean separation between CLI, carriers, encryption, and compression
- **Multiple Carrier Formats**:
  - BMP (24-bit and 32-bit)
  - PNG (RGB/RGBA)
  - WAV (PCM audio)
  - Text (zero-width Unicode)
- **Multiple Encryption Algorithms**:
  - AES-256-GCM
  - ChaCha20-Poly1305
  - XOR (educational/testing only)
  - RSA-Hybrid (planned)
- **Compression**: Optional zlib compression
- **Secure Practices**: PBKDF2 key derivation, authenticated encryption

## Building

### Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- OpenSSL (libssl-dev)
- zlib (zlib1g-dev)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run Tests

```bash
ctest
```

## Usage

### Embed a file into a carrier

```bash
stegtool embed [OPTIONS]
```

### Extract a file from a carrier

```bash
stegtool extract [OPTIONS]
```

### Show help

```bash
stegtool help
stegtool --help
```

## Architecture

The project follows a layered architecture:

```
CLI Layer
    ↓
Application/Service Layer (Orchestration)
    ↓
Payload Serializer
    ↓
Compression
    ↓
Encryption
    ↓
Carrier (Embedded Data)
    ↓
Output File
```

### Key Components

- **Carriers** (`src/carriers/`): Abstract interface for different carrier formats
- **Encryption** (`src/crypto/`): Pluggable encryption algorithms
- **Compression** (`src/compression/`): Compression/decompression layer
- **Payload** (`src/payload/`): Packet serialization and deserialization
- **CLI** (`src/cli/`): Command-line interface
- **Utils** (`src/utils/`): File I/O and byte manipulation utilities

## Development

This project prioritizes:

- Clean architecture and modularity
- Memory safety and explicit ownership
- Secure cryptographic practices
- Meaningful error handling
- Testability
- Minimal unnecessary dependencies

## License

See LICENSE file for details.

## Documentation

For detailed information about each module, see the implementation files with their inline documentation.
