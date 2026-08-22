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
    int index = 0;

    if (buffer.size() < 2) {
        return ReturnCode::MALFORMED_PACKET;
    }

    // deserialize the fixed header
    message_.header.byte = buffer[index++];
    size_t len = Packet::decodeRemainingLength(buffer);
    index += len / 128 + 1; // skip the remaining length bytes

    if (checkProtoNameInvalid(buffer, index, len)) {
        return ReturnCode::MALFORMED_PACKET;
    }
    index += 2 + 4; // skip the protocol name length and the protocol name itself

    uint8_t version = Packet::readByte(buffer, index);
    index += 1;
    if (version != MQTT_PROTOCOL_V31) {
        return ReturnCode::MALFORMED_PACKET;
    }

    message_.byte = Packet::readByte(buffer, index++);
    message_.payload.keep_alive = Packet::readUint16(buffer, index);
    index += 2;

    // Client ID
    index += Packet::readString16(buffer, index, message_.payload.client_id);

    // will topic and message if will flag is set
    if (message_.bits.will) {
        index += Packet::readString16(buffer, index, message_.payload.will_topic);
        // Will Message
        index += Packet::readString16(buffer, index, message_.payload.will_message);
    }

    if (message_.bits.username) {
        index += Packet::readString16(buffer, index, message_.payload.username);
    }
    if (message_.bits.password) {
        index += Packet::readString16(buffer, index, message_.payload.password);
    }

    return ReturnCode::SUCCESS;
}

bool ConnMessage::checkProtoNameInvalid(const std::vector<uint8_t> &buffer, int index, size_t remainingLength) const
{
    uint16_t protoNameLength = Packet::readUint16(buffer, index);

    char protoName[] = "MQTT";
    if (protoNameLength != sizeof(protoName) - 1
        || remainingLength < 2 + protoNameLength) {
        return true;
    }

    for (int i = 0; i < protoNameLength; ++i) {
        if (buffer[index + 2 + i] != protoName[i]) {
            return true;
        }
    }
    return false;
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
