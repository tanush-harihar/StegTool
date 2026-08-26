cmake_minimum_required(VERSION 3.15)
project(stegtool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific settings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${OPENSSL_INCLUDE_DIR})

# Core source files
set(STEGTOOL_SOURCES
    src/main.cpp
    src/cli/cli.cpp
    src/core/exceptions.cpp
    src/carriers/bmp_carrier.cpp
    src/carriers/png_carrier.cpp
    src/carriers/wav_carrier.cpp
    src/carriers/text_carrier.cpp
    src/crypto/aes_gcm.cpp
    src/crypto/chacha20_poly1305.cpp
    src/crypto/xor_encryptor.cpp
    src/crypto/rsa_hybrid.cpp
    src/crypto/kdf.cpp
    src/payload/packet.cpp
    src/payload/serializer.cpp
    src/compression/zlib_compressor.cpp
    src/utils/file_io.cpp
    src/utils/bytes.cpp
)

# Create the main executable
add_executable(stegtool ${STEGTOOL_SOURCES})

# Link libraries
target_link_libraries(stegtool PRIVATE
    OpenSSL::Crypto
    OpenSSL::SSL
    ZLIB::ZLIB
)

# Enable testing
enable_testing()

# Test executable
set(TEST_SOURCES
    tests/test_bmp.cpp
    tests/test_payload.cpp
    tests/test_crypto.cpp
    tests/test_compression.cpp
    tests/test_integration.cpp
)

# Only add tests if test files exist
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_bmp.cpp" OR 
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_payload.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_crypto.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_compression.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_integration.cpp")
    add_executable(stegtool_tests ${TEST_SOURCES})
    target_link_libraries(stegtool_tests PRIVATE
        OpenSSL::Crypto
        OpenSSL::SSL
        ZLIB::ZLIB
    )
    add_test(NAME stegtool_tests COMMAND stegtool_tests)
endif()

# Installation
install(TARGETS stegtool DESTINATION bin)
