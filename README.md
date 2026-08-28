# stegtool - Terminal-Based Steganography Tool

A production-quality educational CLI application for hiding encrypted and compressed binary files inside different carrier formats.

## Features

- **Modular Architecture**: Clean separation between CLI, carriers, encryption, and compression
- **Multiple Carrier Formats**:
  - BMP (24-bit and 32-bit, with proper header and padding preservation)
  - PNG (RGB/RGBA, chunk-aware parsing)
  - WAV (PCM audio with multi-channel support)
  - Text (zero-width Unicode encoding)
- **Multiple Encryption Algorithms**:
  - AES-256-GCM (authenticated encryption with associated data)
  - ChaCha20-Poly1305 (alternative AEAD cipher)
  - XOR (educational/testing only)
  - RSA-Hybrid (planned for asymmetric encryption)
- **Compression**: Optional zlib compression layer
- **Secure Practices**: PBKDF2-SHA256 key derivation (10,000 iterations), authenticated encryption, CRC32 integrity checks
- **Payload Format**: Versioned packet structure with metadata support

## Building

### Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.15+
- Optional: OpenSSL (for AES-GCM, ChaCha20, XOR encryption)
- Optional: zlib (for compression)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

**Note**: OpenSSL and zlib are optional. The project will build without them, disabling encryption and compression features.

### Enable Tests

```bash
cd build
cmake -DBUILD_TESTS=ON ..
cmake --build .
./stegtool_tests  # or Release\stegtool_tests.exe on Windows
```

## Usage

### Embed a file into a carrier (unencrypted)

```bash
stegtool embed --carrier bmp --in cover.bmp --out stego.bmp --payload secret.txt
```

### Embed with AES-GCM encryption

```bash
stegtool embed --carrier bmp --in cover.bmp --out stego.bmp \
  --payload secret.txt --encrypt aes-gcm --passphrase "my-secret-password"
```

### Extract payload from carrier (unencrypted)

```bash
stegtool extract --in stego.bmp --out recovered.txt
```

### Extract and decrypt

```bash
stegtool extract --in stego.bmp --out recovered.txt \
  --encrypt aes-gcm --passphrase "my-secret-password"
```

### Show help

```bash
stegtool help
stegtool --help
stegtool --version
```

## Architecture

The project follows a layered, modular architecture:

```
┌─────────────────────────────────────┐
│  CLI Layer (Command Parsing)        │
├─────────────────────────────────────┤
│  Orchestration Layer                │
│  (Combines carriers, serialization) │
├─────────────────────────────────────┤
│  Payload Packet (Serialization)     │
│  (Magic, version, metadata, CRC)    │
├─────────────────────────────────────┤
│  Compression Layer (Optional zlib)  │
├─────────────────────────────────────┤
│  Encryption Layer (Pluggable)       │
│  (AES-GCM, ChaCha20, XOR, RSA)      │
├─────────────────────────────────────┤
│  Carrier Layer (Format-Specific)    │
│  (BMP, PNG, WAV, Text)              │
├─────────────────────────────────────┤
│  Output File                        │
└─────────────────────────────────────┘
```

### Key Components

- **Carriers** (`src/carriers/`): 
  - Abstract `Carrier` interface
  - Format-specific implementations (BMP, PNG, WAV, Text)
  - `CarrierFactory` for runtime carrier instantiation

- **Encryption** (`src/crypto/`):
  - Abstract `Encryptor` interface
  - Algorithm implementations
  - `EncryptorFactory` for pluggable encryption
  - Centralized KDF (PBKDF2-SHA256) for key derivation

- **Payload** (`src/payload/`):
  - `Packet`: Binary format with magic (STG1), version, flags, encryption metadata, CRC32
  - `Serializer`: Compression/decompression with zlib (when available)

- **CLI** (`src/cli/`):
  - User-facing command-line interface
  - Embed and extract command handlers
  - Error reporting and help messages

- **Utils** (`src/utils/`):
  - File I/O operations
  - Byte array manipulation
  - Exception hierarchy

## Security Notes

- **Passwords are not stored** — they are used only to derive encryption keys via PBKDF2
- **Authenticated encryption** — AES-GCM and ChaCha20 provide both confidentiality and authenticity
- **Payload integrity** — CRC32 checksum protects against accidental corruption
- **No security through obscurity** — steganography is assumed alongside encryption for real security

## Development Status

### Completed

- ✅ BMP carrier (with proper header/padding preservation, top-down support)
- ✅ WAV carrier (RIFF/WAVE parsing, LSB embedding with length prefix for proper extraction)
- ✅ Text carrier (zero-width Unicode encoding)
- ✅ PNG carrier (chunk-aware parsing with zlib-based pixel LSB embedding)
- ✅ Payload packet format with CRC32
- ✅ Serializer/deserializer with optional compression
- ✅ AES-256-GCM encryption
- ✅ ChaCha20-Poly1305 encryption
- ✅ XOR encryption (educational)
- ✅ RSA-Hybrid encryption (RSA-wrapped AES-GCM session key)
- ✅ Centralized KDF (PBKDF2-SHA256, with Argon2id stub)
- ✅ CLI with embed/extract commands
- ✅ Factory patterns for carriers and encryptors
- ✅ Comprehensive unit tests (BMP, payload, carriers)
- ✅ Cryptography tests (AES-GCM, ChaCha20, XOR, wrong password detection, corruption)
- ✅ End-to-end integration tests
- ✅ CI/CD pipeline (GitHub Actions on Linux/Windows/macOS)
- ✅ Enhanced error handling and user messages

### Limitations

- ⚠️ PNG carrier requires zlib for full functionality (no zlib = no embedding, only load)
- ⚠️ RSA-Hybrid requires OpenSSL for actual RSA key wrapping
- ⚠️ Argon2id KDF falls back to PBKDF2 (requires libargon2 for full Argon2id support)
- ⚠️ CLI uses simple arg parsing (not cxxopts)
- ⚠️ No palette/indexed PNG support
- ⚠️ No multi-bit-depth audio support (only 8/16/24/32-bit PCM)

## Design Decisions

1. **Factory Pattern**: Carriers and encryptors are instantiated via factories, allowing runtime selection without compile-time dependencies.

2. **Optional Dependencies**: OpenSSL and zlib are optional to support local development. The code gracefully degrades when unavailable.

3. **Packet Format**: All embedded payloads follow a versioned binary packet format for extensibility and integrity checking.

4. **No Compression by Default**: Compression can be disabled for performance or when data is already compressed.

5. **Capacity Calculation**: Capacity is calculated conservatively to account for carrier structure overhead.

## Testing

Run the test suite:

```bash
cmake --build . --config Release
./Release/stegtool_tests  # Windows
./stegtool_tests          # Linux/macOS
```

Tests validate:
- BMP header preservation and padding handling
- Payload packet serialization and CRC validation
- Carrier factory functionality
- End-to-end embed/extract roundtrips
- Encryption/decryption flows

## License

MIT License - See LICENSE file for details.

## Contributing

For bug reports or feature requests, please submit an issue or pull request with:
- Clear problem description
- Steps to reproduce
- Expected vs. actual behavior
- Test cases if applicable

## References

- [PNG Specification](http://www.libpng.org/pub/png/spec/)
- [WAV File Format](https://en.wikipedia.org/wiki/WAV)
- [AES-GCM](https://en.wikipedia.org/wiki/Galois/Counter_Mode)
- [ChaCha20-Poly1305](https://tools.ietf.org/html/rfc7539)
- [PBKDF2](https://en.wikipedia.org/wiki/PBKDF2)
