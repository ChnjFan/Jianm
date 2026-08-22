/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Andrea Giacomo Baldan All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PACKET_HPP
#define PACKET_HPP

#include <vector>

#include "mqtt.h"

namespace jianm {
namespace protocol {

class Packet {
public:
    // All read functions advance the index past the bytes they consume.
    static uint8_t readByte(const std::vector<uint8_t> &buffer, size_t &index);
    static uint16_t readUint16(const std::vector<uint8_t> &buffer, size_t &index);
    static uint32_t readUint32(const std::vector<uint8_t> &buffer, size_t &index);

    static void readString16(const std::vector<uint8_t> &buffer, size_t &index, std::string &outString);

    static int writeByte(std::vector<uint8_t> &buffer, uint8_t value);
    static int writeUint16(std::vector<uint8_t> &buffer, uint16_t value);
    static int writeUint32(std::vector<uint8_t> &buffer, uint32_t value);

    static int writeString16(std::vector<uint8_t> &buffer, const std::string &value);

    static size_t encodeRemainingLength(std::vector<uint8_t> &buffer, size_t length);
    // Returns the decoded remaining length value and advances index past the
    // remaining-length bytes.
    static size_t decodeRemainingLength(const std::vector<uint8_t> &buffer, size_t &index);


private:
    // tool class to help with encoding and decoding MQTT packets
    // do not instantiate this class directly, use static methods instead
    Packet() = default;
    virtual ~Packet() = default;

};

} // namespace protocol
} // namespace jianm

#endif // PACKET_HPP