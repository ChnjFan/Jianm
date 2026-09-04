/*
 * File: /Codec.cpp
 * Project: protocol
 * Created Date: 2026-08-22 19:27:59
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-04 23:06:50
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

#include <stdexcept>

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

PacketPtr Codec::decode(const std::vector<uint8_t> &buffer)
{
    if (buffer.size() < 2) {
        return nullptr;
    }

    Header header{};
    header.byte = buffer[0];
    return deserializePacket(header.bits.type, buffer);
}

bool Codec::encode(const PacketPtr &pkt, std::vector<uint8_t> &buffer)
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

PacketPtr Codec::deserializePacket(uint8_t type, const std::vector<uint8_t> &buffer)
{
    size_t size = buffer.size();
    if (size < 2) {
        return nullptr;
    }

    if (type < sizeof(decoders_) / sizeof(DeserializeFunc) && decoders_[type]) {
        return decoders_[type](buffer);
    }
    return nullptr;
}

/**
 * @brief 
 * 
 * @param buffer 
 * @return PacketPtr 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 4      |           0000           |
 * |----------|--------------------------------------------------|
 * | Byte 2   |               Remaining Length = 2               |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3   |          Packet Identifier MSB                   |
 * |----------|--------------------------------------------------|
 * | Byte 4   |          Packet Identifier LSB                   |
 * |----------|--------------------------------------------------|
 * |                     (No Payload)                            |
 */
PacketPtr Codec::deserializeAckPacket(const std::vector<uint8_t> &buffer)
{
    size_t index = 0;
    Header header;
    header.byte = readByte(buffer, index);
    const uint8_t flags = header.byte & 0x0f;

    if (PacketType::Pubrel == static_cast<PacketType>(header.bits.type)
        && flags != 0x02) { // PUBREL Qos must be 1
        throw std::runtime_error("PUBREL flags error");
    }
    else if (flags != 0) {
        throw std::runtime_error("PUBLISH ACK flags error");
    }

    PacketPtr packet = std::make_shared<Packet>();
    packet->type = static_cast<PacketType>(header.bits.type);
    auto& ack = packet->body.emplace<AckPacket>();

    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > buffer.size()) {
        // Insufficient cached data to form a complete message
        throw std::runtime_error("PUBLISH ACKPACKET remaining length overflow");
    }
    
    ack.packet_id = readUint16(buffer, index);
    return packet;
}

PacketPtr Codec::deserializeEmptyPacket(const std::vector<uint8_t> &buffer)
{
    size_t index = 0;
    Header header;
    header.byte = readByte(buffer, index);
    const uint8_t flags = header.byte & 0x0f;

    if (flags != 0) {
        throw std::runtime_error("Empty packet flags error");
    }

    PacketPtr packet = std::make_shared<Packet>();
    packet->type = static_cast<PacketType>(header.bits.type);
    packet->body.emplace<EmptyPacket>();

    return packet;
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
    size_t index = 0;
    PacketPtr packet = std::make_shared<Packet>();
    packet->type = PacketType::Connect;
    auto& cp = packet->body.emplace<ConnectPacket>();
    
    Header header;
    header.byte = readByte(buffer, index);
    if ((header.byte & 0x0f) != 0) {
        // The CONNECT Packet Fixed Header Flags (bit 3‑0 of byte1) are Reserved and MUST be 0.
        // If invalid flags are received, the receiver MUST close the Network Connection [MQTT‑2.2.2‑2]
        throw std::runtime_error("CONNECT flags error");
    }

    // remainingLength can be 0, for example, PINREQ.
    // The value is verified at the session layer; only message parsing is performed here.
    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > buffer.size()) {
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
    cp.will_qos = static_cast<Qos>(cp.bits.will_qos);
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
            throw std::runtime_error("CONNECT username is not utf-8");
        }
    }

    if (cp.bits.password) {
        readString16(buffer, index, cp.password);
    }

    return packet;
}

/**
 * @brief Deserialize PUBLISH
 * 
 * @param buffer 
 * @return PacketPtr 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 3      | DUP |    QoS    | RETAIN |
 * |----------|--------------------------------------------------|
 * | Byte 2   |            Remaining Length                      |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3~4 |            Topic Length                          |
 * |----------|--------------------------------------------------|
 * | Byte 5.. |            Topic Name (UTF-8)                    |
 * |----------|--------------------------------------------------|  <-- Packet Identifier (QoS > 0 only)
 * | Byte n+2 |            Packet Identifier                     |
 * |----------|--------------------------------------------------|  <-- Payload
 * | Byte n+3 |            Application Message                   |
 * |   ...    |            (variable length)                     |
 * 
 */
PacketPtr Codec::deserializePublish(const std::vector<uint8_t> &buffer)
{
    size_t index = 0;
    PacketPtr packet = std::make_shared<Packet>();
    packet->type = PacketType::Publish;
    auto& pub = packet->body.emplace<PublishPacket>();

    Header header;
    header.byte = readByte(buffer, index);
    if (header.bits.qos == 3) {
        throw std::runtime_error("PUBLISH qos");
    }

    pub.dup = header.bits.dup;
    pub.qos = static_cast<Qos>(header.bits.qos);
    pub.retain = header.bits.retain;

    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > buffer.size()) {
        // Insufficient cached data to form a complete message
        throw std::runtime_error("PUBLISH remaining length overflow");
    }
    const size_t headerStart = index;

    readString16(buffer, index, pub.topic);
    if (!jianm::common::is_valid_utf8(pub.topic)) {
        throw std::runtime_error("PUBLISH topic is not utf-8");
    }

    if (static_cast<uint8_t>(pub.qos) > static_cast<uint8_t>(Qos::AtMostOnce)) {
        pub.packet_id = readUint16(buffer, index);
    }

    const size_t payloadSize = remainingLength - (index - headerStart);
    pub.payload.assign(reinterpret_cast<const char *>(buffer.data()) + index, payloadSize);

    return packet;
}

PacketPtr Codec::deserializePubAck(const std::vector<uint8_t> &buffer)
{
    return deserializeAckPacket(buffer);
}

PacketPtr Codec::deserializePubRec(const std::vector<uint8_t> &buffer)
{
    return deserializeAckPacket(buffer);
}

PacketPtr Codec::deserializePubRel(const std::vector<uint8_t> &buffer)
{
    return deserializeAckPacket(buffer);
}

PacketPtr Codec::deserializePubComp(const std::vector<uint8_t> &buffer)
{
    return deserializeAckPacket(buffer);
}

/**
 * @brief Deserialize SUBSCRIBE
 * 
 * @param buffer 
 * @return PacketPtr 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 8      |  0  |  0  |  1  |   0    |
 * |----------|--------------------------------------------------|
 * | Byte 2   |            Remaining Length                      |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3   |        Packet Identifier MSB                     |
 * |----------|--------------------------------------------------|
 * | Byte 4   |        Packet Identifier LSB                     |
 * |----------|--------------------------------------------------|  <-- Payload (one or more Topic Filter + QoS)
 * | Byte 5~6 |        Topic Filter Length (MSB/LSB)             |
 * |----------|--------------------------------------------------|
 * | Byte 7.. |        Topic Filter (UTF-8 encoded string)       |
 * |----------|--------------------------------------------------|
 * | Byte n+1 |        Requested QoS                             |
 * |----------|--------------------------------------------------|
 * |   ...    |        (repeats for each Topic Filter)           |
 */
PacketPtr Codec::deserializeSubscribe(const std::vector<uint8_t> &buffer)
{
    size_t index = 0;
    PacketPtr packet = std::make_shared<Packet>();
    packet->type = PacketType::Subscribe;
    auto& sub = packet->body.emplace<SubscribePacket>();

    Header header;
    header.byte = readByte(buffer, index);

    // Specification requirement [MQTT‑3.8.1‑1]:
    // Bits 3, 2, 1 and 0 of the fixed header in the SUBSCRIBE packet
    // are reserved and MUST be set to 0, 0, 1 and 0 respectively;
    // otherwise, the server MUST treat it as malformed and close the connection.
    if (header.byte != MQTT_SUBSCRIBE_BYTE) {
        throw std::runtime_error("SUBSCRIBE flags");
    }

    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > buffer.size()) {
        throw std::runtime_error("SUBSCRIBE remaining length overflow");
    }

    const size_t packetEnd = index + remainingLength;

    sub.packet_id = readUint16(buffer, index);
    while (index < packetEnd) {
        SubscribeEntry entry;
        readString16(buffer, index, entry.filter);
        if (!jianm::common::is_valid_utf8(entry.filter)) {
            throw std::runtime_error("SUBSCRIBE topic filter is not utf-8");
        }
        entry.qos = static_cast<Qos>(readByte(buffer, index));
        if (static_cast<uint8_t>(entry.qos) > static_cast<uint8_t>(Qos::ExactlyOnce)) {
            throw std::runtime_error("SUBSCRIBE qos");
        }
        sub.entries.push_back(std::move(entry));
    }
    return packet;
}

/**
 * @brief Deserialize UNSUBSCRIBE
 * 
 * @param buffer 
 * @return PacketPtr 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 10     |  0  |  0  |  1  |   0    |
 * |----------|--------------------------------------------------|
 * | Byte 2   |            Remaining Length                      |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3   |        Packet Identifier MSB                     |
 * |----------|--------------------------------------------------|
 * | Byte 4   |        Packet Identifier LSB                     |
 * |----------|--------------------------------------------------|  <-- Payload (one or more Topic Filter)
 * | Byte 5~6 |        Topic Filter Length (MSB/LSB)             |
 * |----------|--------------------------------------------------|
 * | Byte 7.. |        Topic Filter (UTF-8 encoded string)       |
 * |----------|--------------------------------------------------|
 * |   ...    |        (repeats for each Topic Filter)           |
 */
PacketPtr Codec::deserializeUnsubscribe(const std::vector<uint8_t> &buffer)
{
    size_t index = 0;
    PacketPtr packet = std::make_shared<Packet>();
    packet->type = PacketType::Unsubscribe;
    auto& unSub = packet->body.emplace<UnsubscribePacket>();

    Header header;
    header.byte = readByte(buffer, index);

    if (header.byte != MQTT_UNSUBSCRIBE_BYTE) {
        throw std::runtime_error("UNSUBSCRIBE flags");
    }

    size_t remainingLength = decodeRemainingLength(buffer, index);
    if (index + remainingLength > buffer.size()) {
        throw std::runtime_error("UNSUBSCRIBE remaining length overflow");
    }

    const size_t packetEnd = index + remainingLength;

    unSub.packet_id = readUint16(buffer, index);
    while (index < packetEnd) {
        std::string filter;
        readString16(buffer, index, filter);
        if (!jianm::common::is_valid_utf8(filter)) {
            throw std::runtime_error("UNSUBSCRIBE topic filter is not utf-8");
        }
        unSub.topics.push_back(std::move(filter));
    }
    return packet;
}

PacketPtr Codec::deserializePingReq(const std::vector<uint8_t> &buffer)
{
    return deserializeEmptyPacket(buffer);
}

PacketPtr Codec::deserializePingResp(const std::vector<uint8_t> &buffer)
{
    return deserializeEmptyPacket(buffer);
}

PacketPtr Codec::deserializeDisconnect(const std::vector<uint8_t> &buffer)
{
    return deserializeEmptyPacket(buffer);
}

bool Codec::serializePacket(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    auto type = static_cast<uint8_t>(pkt->type);
    if (type < sizeof(encoders_) / sizeof(SerializeFunc) && encoders_[type]) {
        return encoders_[type](pkt, buffer);
    }
    return false;
}

bool Codec::serializeAckPacket(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    if (pkt->type == PacketType::Pubrel) {
        writeByte(buffer, MQTT_PUBREL_BYTE);
    }
    else {
        writeByte(buffer, static_cast<uint8_t>(static_cast<uint8_t>(pkt->type) << 4));
    }
    writeByte(buffer, 2); // remaining length
    const auto& ack = std::get<AckPacket>(pkt->body);
    writeUint16(buffer, ack.packet_id);

    return true;
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
bool Codec::serializeConnack(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    // CONNACK has a fixed length of 4 bytes
    buffer.reserve(4);
    writeByte(buffer, MQTT_CONNACK_BYTE);
    // [MQTT-3.2.1]For the CONNACK Packet this has the value 2
    writeByte(buffer, 2);
    const auto& ca = std::get<ConnackPacket>(pkt->body);
    writeByte(buffer, ca.session_present ? 1 : 0);
    writeByte(buffer, static_cast<uint8_t>(ca.return_code));

    return true;
}

bool Codec::serializePublish(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    Header header{0};
    header.bits.type = static_cast<uint8_t>(PacketType::Publish);
    const auto& pub = std::get<PublishPacket>(pkt->body);
    header.bits.dup = pub.dup ? 1 : 0;
    header.bits.retain = pub.retain ? 1 : 0;
    header.bits.qos = static_cast<uint8_t>(pub.qos);
    writeByte(buffer, header.byte);
    

    size_t remainingLength = calcPublishRemainingLength(pub);
    if (remainingLength == 0) {
        return false;
    }
    encodeRemainingLength(buffer, remainingLength);

    writeString16(buffer, pub.topic);

    if (pub.qos != Qos::AtMostOnce) {
        writeUint16(buffer, pub.packet_id);
    }
    
    buffer.insert(buffer.end(), pub.payload.begin(), pub.payload.end());
    return true;
}

bool Codec::serializePubAck(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    return serializeAckPacket(pkt, buffer);
}

bool Codec::serializePubRec(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    return serializeAckPacket(pkt, buffer);
}

bool Codec::serializePubRel(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    return serializeAckPacket(pkt, buffer);
}

bool Codec::serializePubComp(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    return serializeAckPacket(pkt, buffer);
}

bool Codec::serializeSubAck(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    const auto& sa = std::get<SubackPacket>(pkt->body);
    writeByte(buffer, MQTT_SUBACK_BYTE);
    encodeRemainingLength(buffer, 2 + sa.granted.size());
    writeUint16(buffer, sa.packet_id);
    for (const auto& qos : sa.granted) {
        writeByte(buffer, qos);
    }
    return true;
}

/**
 * @brief Serialize the UNSUBACK packet
 * 
 * @param pkt 
 * @param buffer 
 * @return true 
 * @return false 
 * 
 * |   Bit    |  7  |  6  |  5  |  4  |  3  |  2  |  1  |   0    |  <-- Fixed Header
 * |----------|-----------------------|--------------------------|
 * | Byte 1   |    MQTT type = 11     |  0  |  0  |  0  |   0    |
 * |----------|--------------------------------------------------|
 * | Byte 2   |            Remaining Length = 2                  |
 * |----------|--------------------------------------------------|  <-- Variable Header
 * | Byte 3   |        Packet Identifier MSB                     |
 * |----------|--------------------------------------------------|
 * | Byte 4   |        Packet Identifier LSB                     |
 * |----------|--------------------------------------------------|
 * |                     (No Payload)                            |

 */
bool Codec::serializeUnsubAck(PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    const auto& ua = std::get<AckPacket>(pkt->body);
    writeByte(buffer, MQTT_UNSUBACK_BYTE);
    writeByte(buffer, 2); // remaining length
    writeUint16(buffer, ua.packet_id);
    return true;
}

bool Codec::serializePingResp([[maybe_unused]]PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    writeByte(buffer, MQTT_PINGRESP_BYTE);
    writeByte(buffer, 0); // remaining length
    return true;
}

bool Codec::serializeDisconnect([[maybe_unused]]PacketPtr pkt, std::vector<uint8_t> &buffer)
{
    writeByte(buffer, MQTT_DISCONNECT_BYTE);
    writeByte(buffer, 0); // remaining length
    return true;
}

size_t Codec::calcPublishRemainingLength(const PublishPacket &pkt)
{
    int length = 0;

    length += (2 + pkt.topic.length());
    if (pkt.qos != Qos::AtMostOnce) {
        length += 2;
    }

    length += pkt.payload.size();

    return length;
}
