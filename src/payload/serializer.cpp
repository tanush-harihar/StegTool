#include "serializer.hpp"
#include "packet.hpp"
#include "../core/exceptions.hpp"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

namespace stegtool::payload {

Packet Serializer::serialize_with_compression(const ByteArray& data, bool compress) {
#ifdef HAVE_ZLIB
    if (compress && !data.empty()) {
        // worst-case compressed size
        uLongf dest_len = compressBound(static_cast<uLong>(data.size()));
        ByteArray dest(dest_len);
        int rc = ::compress2(dest.data(), &dest_len, data.data(), static_cast<uLong>(data.size()), Z_BEST_COMPRESSION);
        if (rc != Z_OK) {
            throw stegtool::CompressionException("zlib compress2 failed");
        }
        dest.resize(dest_len);
        Packet p(dest);
        // mark compression method in enc_meta as 'zlib'
        p.set_encryption_metadata(ByteArray({'z','l','i','b'}));
        return p;
    }
#endif
    // Fallback: no compression performed
    return Packet(data);
}

ByteArray Serializer::deserialize_with_decompression(const Packet& packet) {
    const ByteArray& enc_meta = packet.encryption_metadata();
    if (!enc_meta.empty()) {
        // check for zlib marker
        if (enc_meta.size() == 4 && enc_meta[0]=='z' && enc_meta[1]=='l' && enc_meta[2]=='i' && enc_meta[3]=='b') {
#ifdef HAVE_ZLIB
            const ByteArray& src = packet.data();
            // attempt to decompress with an initial guess and grow if needed
            uLongf dest_len = src.size() * 4 + 1024; // heuristic
            ByteArray dest;
            int rc = Z_BUF_ERROR;
            for (int attempts = 0; attempts < 6 && rc == Z_BUF_ERROR; ++attempts) {
                dest.resize(dest_len);
                rc = ::uncompress(dest.data(), &dest_len, src.data(), static_cast<uLong>(src.size()));
                if (rc == Z_OK) break;
                if (rc == Z_BUF_ERROR) dest_len *= 2;
            }
            if (rc != Z_OK) {
                throw stegtool::CompressionException("zlib uncompress failed");
            }
            dest.resize(dest_len);
            return dest;
#else
            // zlib metadata present but zlib not available: error
            throw stegtool::CompressionException("zlib metadata present but library not available");
#endif
        }
    }
    // no compression metadata: return raw data
    return packet.data();
}

} // namespace stegtool::payload
