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

#include "ConnMessage.hpp"
#include "Packet.hpp"

using namespace jianm::protocol;

ReturnCode ConnMessage::serialize([[maybe_unused]] std::vector<uint8_t> &buffer) const
{
    // Broker not need to serialize CONNECT message, only need to deserialize it
    return ReturnCode::SUCCESS;
}

ReturnCode ConnMessage::deserialize(const std::vector<uint8_t> &buffer)
{
    if (buffer.size() < 2) {
        return ReturnCode::MALFORMED_PACKET;
    }

    size_t index = 0;
    // Fixed header: packet type byte
    message_.header.byte = Packet::readByte(buffer, index);

    // Remaining length (variable-length encoding, advances index past it)
    size_t remainingLength = Packet::decodeRemainingLength(buffer, index);
    if (remainingLength == 0) {
        return ReturnCode::MALFORMED_PACKET;
    }

    // Protocol Name (2-byte length + "MQTT")
    std::string protoName;
    Packet::readString16(buffer, index, protoName);
    if (protoName != "MQTT") {
        return ReturnCode::MALFORMED_PACKET;
    }

    // Protocol Level
    uint8_t version = Packet::readByte(buffer, index);
    if (version != MQTT_PROTOCOL_V31) {
        return ReturnCode::PROTOCOL_VERSION_NOT_SUPPORT;
    }

    // Connect Flags (clean_session, will, qos, retain, password, username)
    message_.byte = Packet::readByte(buffer, index);
    message_.payload.keep_alive = Packet::readUint16(buffer, index);
    Packet::readString16(buffer, index, message_.payload.client_id);

    if (message_.bits.will) {
        Packet::readString16(buffer, index, message_.payload.will_topic);
        Packet::readString16(buffer, index, message_.payload.will_message);
    }

    if (message_.bits.username) {
        Packet::readString16(buffer, index, message_.payload.username);
    }

    if (message_.bits.password) {
        Packet::readString16(buffer, index, message_.payload.password);
    }

    return ReturnCode::SUCCESS;
}

ReturnCode ConnAckMessage::serialize(std::vector<uint8_t> &buffer) const
{
    Packet::writeByte(buffer, message_.header.byte);
    // CONNACK remaining length is always 2 bytes
    uint8_t remainingLength = 2;
    Packet::encodeRemainingLength(buffer, remainingLength);
    Packet::writeByte(buffer, message_.byte);
    Packet::writeByte(buffer, message_.return_code);
    return ReturnCode::SUCCESS;
}

ReturnCode ConnAckMessage::deserialize([[maybe_unused]] const std::vector<uint8_t> &buffer)
{
    // Broker not need to deserialize CONNACK message, only need to serialize it
    return ReturnCode::SUCCESS;
}
