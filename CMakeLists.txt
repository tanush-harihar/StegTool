cmake_minimum_required(VERSION 3.15)
project(stegtool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(OpenSSL)
find_package(ZLIB)

# Include directories and defines
include_directories(${CMAKE_SOURCE_DIR}/src)
if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    add_compile_definitions(HAVE_OPENSSL=1)
endif()
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    add_compile_definitions(HAVE_ZLIB=1)
endif()

# Core source files
set(STEGTOOL_SOURCES
    src/main.cpp
    src/cli/cli.cpp
    src/core/exceptions.cpp
    src/carriers/bmp_carrier.cpp
    src/carriers/png_carrier.cpp
    src/carriers/wav_carrier.cpp
    src/carriers/text_carrier.cpp
    src/carriers/carrier_factory.cpp
    src/payload/packet.cpp
    src/payload/serializer.cpp
    src/utils/file_io.cpp
    src/utils/bytes.cpp
)

# Always include noop encryptor and factory
list(APPEND STEGTOOL_SOURCES
    src/crypto/noop_encryptor.cpp
    src/crypto/encryptor_factory.cpp
)

# Add OpenSSL-based crypto if available
if(OpenSSL_FOUND)
    list(APPEND STEGTOOL_SOURCES
        src/crypto/aes_gcm.cpp
        src/crypto/chacha20_poly1305.cpp
        src/crypto/xor_encryptor.cpp
        src/crypto/rsa_hybrid.cpp
        src/crypto/kdf.cpp
    )
endif()

# Add zlib compressor if available
if(ZLIB_FOUND)
    list(APPEND STEGTOOL_SOURCES src/compression/zlib_compressor.cpp)
endif()

# Create the main executable
add_executable(stegtool ${STEGTOOL_SOURCES})

# Link libraries for stegtool
if(OpenSSL_FOUND AND ZLIB_FOUND)
    target_link_libraries(stegtool PRIVATE OpenSSL::Crypto OpenSSL::SSL ZLIB::ZLIB)
elseif(OpenSSL_FOUND)
    target_link_libraries(stegtool PRIVATE OpenSSL::Crypto OpenSSL::SSL)
elseif(ZLIB_FOUND)
    target_link_libraries(stegtool PRIVATE ZLIB::ZLIB)
endif()

# Enable testing
enable_testing()

# Base sources for all test executables
set(TEST_BASE_SOURCES
    src/carriers/bmp_carrier.cpp
    src/carriers/png_carrier.cpp
    src/carriers/wav_carrier.cpp
    src/carriers/text_carrier.cpp
    src/carriers/carrier_factory.cpp
    src/utils/file_io.cpp
    src/core/exceptions.cpp
    src/utils/bytes.cpp
    src/payload/packet.cpp
    src/payload/serializer.cpp
)

# Build test_bmp as the main test executable (also links test_payload and test_e2e for cross-test calls)
add_executable(stegtool_tests tests/test_bmp.cpp tests/test_payload.cpp tests/test_e2e.cpp ${TEST_BASE_SOURCES})

# Build test_crypto as a separate executable
add_executable(stegtool_crypto_tests tests/test_crypto.cpp ${TEST_BASE_SOURCES} src/crypto/noop_encryptor.cpp)
if(OpenSSL_FOUND)
    target_sources(stegtool_crypto_tests PRIVATE
        src/crypto/aes_gcm.cpp
        src/crypto/chacha20_poly1305.cpp
        src/crypto/xor_encryptor.cpp
        src/crypto/rsa_hybrid.cpp
        src/crypto/kdf.cpp
    )
    target_link_libraries(stegtool_crypto_tests PRIVATE OpenSSL::Crypto OpenSSL::SSL)
endif()

# Build test_integration as a separate executable
add_executable(stegtool_integration_tests tests/test_integration.cpp ${TEST_BASE_SOURCES} src/crypto/noop_encryptor.cpp)
if(OpenSSL_FOUND)
    target_sources(stegtool_integration_tests PRIVATE
        src/crypto/aes_gcm.cpp
        src/crypto/chacha20_poly1305.cpp
        src/crypto/xor_encryptor.cpp
        src/crypto/rsa_hybrid.cpp
        src/crypto/kdf.cpp
    )
    target_link_libraries(stegtool_integration_tests PRIVATE OpenSSL::Crypto OpenSSL::SSL)
endif()

# Add tests to ctest
add_test(NAME stegtool_tests COMMAND stegtool_tests)
add_test(NAME stegtool_crypto_tests COMMAND stegtool_crypto_tests)
add_test(NAME stegtool_integration_tests COMMAND stegtool_integration_tests)

# Installation
install(TARGETS stegtool DESTINATION bin)
