cmake_minimum_required(VERSION 3.15)
project(stegtool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific settings
option(BUILD_TESTS "Build tests" OFF)
if(MSVC)
    # High warning level but do not treat warnings as errors for now
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages (OpenSSL optional for local builds)
find_package(OpenSSL)
find_package(ZLIB)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
if(OpenSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    add_compile_definitions(HAVE_OPENSSL=1)
endif()
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    # expose HAVE_ZLIB to the compiler
    add_compile_definitions(HAVE_ZLIB=1)
endif()

# Core source files (crypto and zlib added conditionally)
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

# Append crypto sources
list(APPEND STEGTOOL_SOURCES
    src/crypto/noop_encryptor.cpp
    src/crypto/encryptor_factory.cpp
)

# Append crypto implementations that require OpenSSL only if available
if(OpenSSL_FOUND)
    list(APPEND STEGTOOL_SOURCES
        src/crypto/aes_gcm.cpp
        src/crypto/chacha20_poly1305.cpp
        src/crypto/xor_encryptor.cpp
        src/crypto/rsa_hybrid.cpp
        src/crypto/kdf.cpp
    )
endif()

# Add tests for encryptor factory
if(BUILD_TESTS)
        list(APPEND TEST_SOURCES src/crypto/encryptor_factory.cpp src/crypto/noop_encryptor.cpp)
endif()

# Append zlib compressor only if ZLIB is available
if(ZLIB_FOUND)
    list(APPEND STEGTOOL_SOURCES src/compression/zlib_compressor.cpp)
endif()

# Create the main executable
add_executable(stegtool ${STEGTOOL_SOURCES})

# Link libraries
if(OpenSSL_FOUND)
    if(ZLIB_FOUND)
        target_link_libraries(stegtool PRIVATE
            OpenSSL::Crypto
            OpenSSL::SSL
            ZLIB::ZLIB
        )
    else()
        target_link_libraries(stegtool PRIVATE
            OpenSSL::Crypto
            OpenSSL::SSL
        )
    endif()
else()
    if(ZLIB_FOUND)
        target_link_libraries(stegtool PRIVATE ZLIB::ZLIB)
    else()
        # No external libraries found; proceed with minimal linking
        target_link_libraries(stegtool PRIVATE)
    endif()
endif()

# Enable testing (off by default)
enable_testing()

# Test executable (BMP + payload tests)
set(TEST_SOURCES
    tests/test_bmp.cpp
    tests/test_payload.cpp
    tests/test_carrier_factory.cpp
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

# Only add tests if BUILD_TESTS is ON and test files exist
if(BUILD_TESTS AND (EXISTS "${CMAKE_SOURCE_DIR}/tests/test_bmp.cpp" OR 
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_payload.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_crypto.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_compression.cpp" OR
   EXISTS "${CMAKE_SOURCE_DIR}/tests/test_integration.cpp"))
    add_executable(stegtool_tests ${TEST_SOURCES})
    if(OpenSSL_FOUND)
        if(ZLIB_FOUND)
            target_link_libraries(stegtool_tests PRIVATE
                OpenSSL::Crypto
                OpenSSL::SSL
                ZLIB::ZLIB
            )
        else()
            target_link_libraries(stegtool_tests PRIVATE
                OpenSSL::Crypto
                OpenSSL::SSL
            )
        endif()
    else()
        if(ZLIB_FOUND)
            target_link_libraries(stegtool_tests PRIVATE ZLIB::ZLIB)
        else()
            target_link_libraries(stegtool_tests PRIVATE)
        endif()
    endif()
    add_test(NAME stegtool_tests COMMAND stegtool_tests)
endif()

# Installation
install(TARGETS stegtool DESTINATION bin)
