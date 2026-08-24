/*
 * File: /Codec.cpp
 * Project: protocol
 * Created Date: 2026-08-22 19:27:59
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 14:37:01
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


#include "Codec.hpp"
#include "common/Utils.hpp"

using namespace jianm::protocol;

// remaining length can be encoded in at most 4 bytes
static constexpr size_t MAX_REMAINING_SIZE = 4;

size_t Codec::encodeRemainingLength(std::vector<uint8_t> &buffer, size_t length)
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

size_t Codec::decodeRemainingLength(const std::vector<uint8_t> &buffer, size_t &index)
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

PacketPtr jianm::protocol::Codec::decode(const std::vector<uint8_t> &buffer)
{
    if (buffer.size() < 2) {
        return nullptr;
    }

    jianm::broker::Header header{ .byte = buffer[0] };
    return deserializePacket(header.bits.type, buffer);
}

bool jianm::protocol::Codec::encode(const PacketPtr &pkt, std::vector<uint8_t> &buffer)
{
    return serializePacket(pkt, buffer);
}

uint8_t Codec::readByte(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint8_t value = buffer[index];
    index += 1;
    return value;
}

uint16_t Codec::readUint16(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint16_t value = (static_cast<uint16_t>(buffer[index]) << 8) |
                     static_cast<uint16_t>(buffer[index + 1]);
    index += 2;
    return value;
}

uint32_t Codec::readUint32(const std::vector<uint8_t> &buffer, size_t &index)
{
    uint32_t value = (static_cast<uint32_t>(buffer[index]) << 24) |
                     (static_cast<uint32_t>(buffer[index + 1]) << 16) |
                     (static_cast<uint32_t>(buffer[index + 2]) << 8) |
                     static_cast<uint32_t>(buffer[index + 3]);
    index += 4;
    return value;
}

void Codec::readString16(const std::vector<uint8_t> &buffer, size_t &index, std::string &outString)
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

int Codec::writeByte(std::vector<uint8_t> &buffer, uint8_t value)
{
    buffer.push_back(value);
    return 1;
}

int Codec::writeUint16(std::vector<uint8_t> &buffer, uint16_t value)
{
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    return 2;
}

int Codec::writeUint32(std::vector<uint8_t> &buffer, uint32_t value)
{
    buffer.push_back(static_cast<uint8_t>(value >> 24));
    buffer.push_back(static_cast<uint8_t>(value >> 16));
    buffer.push_back(static_cast<uint8_t>(value >> 8));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    return 4;
}

int Codec::writeString16(std::vector<uint8_t> &buffer, const std::string &value)
{
    int bytesWritten = writeUint16(buffer, static_cast<uint16_t>(value.length()));
    for (char c : value) {
        bytesWritten += writeByte(buffer, static_cast<uint8_t>(c));
    }
    return bytesWritten;
}

PacketPtr jianm::protocol::Codec::deserializePacket(uint8_t type, const std::vector<uint8_t> &buffer)
{
    if (type < sizeof(decoders_) / sizeof(DeserializeFunc) && decoders_[type]) {
        return decoders_[type](buffer);
    }
    return nullptr;
}

/**
 * @brief Deserializing the CONNECT message
 * 
 * @param buffer 
 * @return PacketPtr 
 * 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |
 * |----------|-----------------------|--------------------------|  <-- Fixed Header
 * | Byte 1   |    MQTT type = 1      | 0   |    00     |   0    |
 * |----------|--------------------------------------------------|
 * | Byte 2   |               Remaining Length = 19              |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3~4 |           Protocol Name Length = 0x00 0x04       |
 * |----------|--------------------------------------------------|
 * | Byte 5~8 |                  "MQTT"                          |
 * |----------|--------------------------------------------------|
 * | Byte 9   |           Protocol Level = 4 (0x04)              |
 * |----------|--------------------------------------------------|
 * | Byte 10  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |  0     |
 * |          |  │     │     │     │     │     │     │     └─────┤── Clean Session = 1
 * |          |  │     │     │     │     │     │     └───────────┤── Will Flag = 0
 * |          |  │     │     │     │     │     └─────────────────┤── Will QoS = 00
 * |          |  │     │     │     │     └───────────────────────┤── Will Retain = 0
 * |          |  │     │     │     └─────────────────────────────┤── Password = 0
 * |          |  │     │     └───────────────────────────────────┤── Username = 0
 * |          |  │     └─────────────────────────────────────────┤── Reserved = 0
 * |          |  └───────────────────────────────────────────────┤── Reserved = 0
 * |----------|--------------------------------------------------|
 * |Byte 11~12|           Keep Alive = 0x00 0x3C (60s)           |
 * |----------|--------------------------------------------------|  <-- Payload
 * |Byte 13~14|           Client ID Length = 0x00 0x0A (10)      |
 * |----------|--------------------------------------------------|
 * |Byte 15~24|           "testclient"                           |
 * |----------|--------------------------------------------------|
 *               ... will topic/will message/username/password
 */
PacketPtr Codec::deserializeConnect(const std::vector<uint8_t> &buffer)
{
    size_t size = buffer.size();
    if (size < 2) {
        return nullptr;
    }

    size_t index = 0;
    PacketPtr packet = std::make_shared<jianm::broker::Packet>();
    packet->type = jianm::broker::PacketType::Connect;
    auto& cp = packet->body.emplace<jianm::broker::ConnectPacket>();
    
    jianm::broker::Header header;
    header.byte = readByte(buffer, index);
    if ((header.byte & 0x0f) != 0) {
        // The CONNECT Packet Fixed Header Flags (bit 3‑0 of byte1) are Reserved and MUST be 0.
        // If invalid flags are received, the receiver MUST close the Network Connection [MQTT‑2.2.2‑2]
        throw std::runtime_error("CONNECT flags error");
    }

    // remainingLength can be 0, for example, PINREQ.
    // The value is verified at the session layer; only message parsing is performed here.
    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > size) {
        // Insufficient cached data to form a complete message
        throw std::runtime_error("CONNECT remaining length overflow");
    }

    // Protocol Name (2-byte length + "MQTT")
    readString16(buffer, index, cp.protocol);
    if (!jianm::common::is_valid_utf8(cp.protocol)) {
        throw std::runtime_error("CONNECT protocol is not utf-8");
    }

    cp.level = readByte(buffer, index);
    cp.byte = readByte(buffer, index);
    if (cp.bits.reserved != 0) {
        // Specification requirement [MQTT‑3.1.2‑3]:
        // The Reserved bit (bit 0) in the CONNECT packet MUST be 0;
        // otherwise, the server MUST close the connection.
        throw std::runtime_error("CONNECT protocol reserved is not 0");
    }

    cp.clean_session = cp.bits.clean_session;
    cp.has_will = cp.bits.will;
    cp.will_qos = static_cast<jianm::broker::Qos>(cp.bits.will_qos);
    cp.will_retain = cp.bits.will_retain;
    if (cp.has_will && cp.bits.will_qos == 3) {
        throw std::runtime_error("CONNECT protocol will qos is 3");
    }
    if (!cp.has_will && (cp.will_retain || cp.bits.will_qos != 0)) {
        throw std::runtime_error("CONNECT protocol will flags without will");
    }
    
    cp.keepalive = readUint16(buffer, index);
    readString16(buffer, index, cp.client_id);
    if (!jianm::common::is_valid_utf8(cp.client_id)) {
        throw std::runtime_error("CONNECT client id is not utf-8");
    }

    if (cp.has_will) {
        readString16(buffer, index, cp.will_topic);
        readString16(buffer, index, cp.will_payload);
    }
    
    if (cp.bits.username) {
        readString16(buffer, index, cp.username);
        if (!jianm::common::is_valid_utf8(cp.username)) {
            throw std::runtime_error("CONNECT username id is not utf-8");
        }
    }

    if (cp.bits.password) {
        readString16(buffer, index, cp.password);
    }

    return packet;
}

bool jianm::protocol::Codec::serializePacket(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    auto type = static_cast<uint8_t>(pkt->type);
    if (type < sizeof(encoders_) / sizeof(SerializeFunc) && encoders_[type]) {
        return encoders_[type](pkt, buffer);
    }
    return false;
}

/**
 * @brief Serialize the CONNACK packet
 * 
 * @param pkt 
 * @param buffer 
 * @return true 
 * @return false 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 2      | 0   |    00     |   0    |
 * |----------|--------------------------------------------------|
 * | Byte 2   |               Remaining Length = 2               |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3   |                 0                       |   SP   |
 * |          |                 │                            └───┤── Session Present (SP)
 * |          |                 └────────────────────────────────┤── Reserved = 0
 * |----------|--------------------------------------------------|
 * | Byte 4   |              Connect Return Code                 |
 * |----------|--------------------------------------------------|
 * |                     (No Payload)                            |
 */
bool jianm::protocol::Codec::serializeConnack(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    // CONNACK has a fixed length of 4 bytes
    buffer.reserve(4);
    buffer.push_back(jianm::broker::MQTT_CONNACK_BYTE);
    // [MQTT-3.2.1]For the CONNACK Packet this has the value 2
    buffer.push_back(2);
    const auto& ca = std::get<jianm::broker::ConnackPacket>(pkt->body);
    buffer.push_back(ca.session_present ? 1 : 0);
    buffer.push_back(static_cast<uint8_t>(ca.return_code));

    return true;
}
