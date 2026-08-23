/*
 * File: /Packet.cpp
 * Project: protocol
 * Created Date: 2026-08-22 19:27:59
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 12:57:07
 * Modified By: ChnjFan
 * -----
 * Copyright (c) 2026 ChnjFan
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * -----
 * HISTORY:
 */



#include "Packet.hpp"

using namespace jianm::protocol;

// remaining length can be encoded in at most 4 bytes
static constexpr size_t MAX_REMAINING_SIZE = 4;

uint8_t Packet::readByte(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint8_t value = buffer[index];
    index += 1;
    return value;
}

uint16_t Packet::readUint16(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint16_t value = (static_cast<uint16_t>(buffer[index]) << 8) |
                     static_cast<uint16_t>(buffer[index + 1]);
    index += 2;
    return value;
}

uint32_t Packet::readUint32(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint32_t value = (static_cast<uint32_t>(buffer[index]) << 24) |
                     (static_cast<uint32_t>(buffer[index + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[index + 2]) << 8) |
                     static_cast<uint32_t>(buffer[index + 3]);
    index += 4;
    return value;
}

void Packet::readString16(const std::vector<uint8_t> &buffer, size_t &index, std::string &outString)
{
    if (index + 2 > buffer.size()) {
        return;
    }

    uint16_t length = readUint16(buffer, index);  // advances index by 2
    if (index + length > buffer.size() || length == 0) {
        return;
    }
    outString.assign(buffer.begin() + index, buffer.begin() + index + length);
    index += length;
}

int Packet::writeByte(std::vector<uint8_t> &buffer, uint8_t value)
{
    buffer.push_back(value);
    return 1;
}

int Packet::writeUint16(std::vector<uint8_t> &buffer, uint16_t value)
{
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    return 2;
}

int Packet::writeUint32(std::vector<uint8_t> &buffer, uint32_t value)
{
    buffer.push_back(static_cast<uint8_t>(value >> 24));
    buffer.push_back(static_cast<uint8_t>(value >> 16));
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    return 4;
}

int Packet::writeString16(std::vector<uint8_t> &buffer, const std::string &value)
{
    int bytesWritten = writeUint16(buffer, static_cast<uint16_t>(value.length()));
    for (char c : value) {
        bytesWritten += writeByte(buffer, static_cast<uint8_t>(c));
    }
    return bytesWritten;
}

size_t Packet::encodeRemainingLength(std::vector<uint8_t> &buffer, size_t length)
{
    size_t bytes = 0;
    // remaining length is started from the second byte of the fixed header
    // buffer should include the first byte of the fixed header before calling this function
    if (buffer.size() != 1) {
        return 0;
    }

    do {
        uint8_t encoded_byte = length % 128;
        length /= 128;
        // if there are more data to encode, set the top bit of this byte
        if (length > 0) {
            encoded_byte |= 0x80;
        }
        buffer.push_back(encoded_byte);
        bytes++;
    } while (length > 0 && bytes < MAX_REMAINING_SIZE);
    return bytes;
}

size_t Packet::decodeRemainingLength(const std::vector<uint8_t> &buffer, size_t &index)
{
    size_t length = 0;
    size_t multiplier = 1;

    for (size_t i = 0; i < MAX_REMAINING_SIZE; ++i) {
        if (index >= buffer.size()) {
            return 0;
        }
        uint8_t byte = buffer[index++];
        length += (byte & 0x7F) * multiplier;
        multiplier *= 128;
        // first bit of each byte indicates if there are more bytes to read
        if (!(byte & 0x80)) {
            break;
        }
    }
    return length;
}
