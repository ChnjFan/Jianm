/*
 * File: /Codec.hpp
 * Project: protocol
 * Created Date: 2026-08-22 19:27:35
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-31 10:28:07
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

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "jianm/model/Packet.hpp"

namespace jianm {
namespace protocol {

using PacketPtr = std::shared_ptr<Packet>;
using DeserializeFunc = std::function<PacketPtr (const std::vector<uint8_t>& buffer)>;
using SerializeFunc = std::function<bool (PacketPtr, std::vector<uint8_t>& buffer)>;

class Codec {
public:
    static size_t encodeRemainingLength(std::vector<uint8_t> &buffer, size_t length);
    // Returns the decoded remaining length value and advances index past the
    // remaining-length bytes.
    static size_t decodeRemainingLength(const std::vector<uint8_t> &buffer, size_t &index);

    static PacketPtr decode(const std::vector<uint8_t>& buffer);
    static bool encode(const PacketPtr& pkt, std::vector<uint8_t>& buffer);

private:
    // tool class to help with encoding and decoding MQTT packets
    // do not instantiate this class directly, use static methods instead
    Codec() = default;
    virtual ~Codec() = default;

    // All read functions advance the index past the bytes they consume.
    static uint8_t readByte(const std::vector<uint8_t> &buffer, size_t &index);
    static uint16_t readUint16(const std::vector<uint8_t> &buffer, size_t &index);
    static uint32_t readUint32(const std::vector<uint8_t> &buffer, size_t &index);
    static void readString16(const std::vector<uint8_t> &buffer, size_t &index, std::string &outString);

    static int writeByte(std::vector<uint8_t> &buffer, uint8_t value);
    static int writeUint16(std::vector<uint8_t> &buffer, uint16_t value);
    static int writeUint32(std::vector<uint8_t> &buffer, uint32_t value);

    static int writeString16(std::vector<uint8_t> &buffer, const std::string &value);

    static PacketPtr deserializePacket(uint8_t type, const std::vector<uint8_t>& buffer);
    static PacketPtr deserializeAckPacket(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializeConnect(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializePublish(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializePubAck(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializePubRec(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializePubRel(const std::vector<uint8_t>& buffer);
    static PacketPtr deserializePubComp(const std::vector<uint8_t>& buffer);

    static bool serializePacket(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializeAckPacket(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializeConnack(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializePublish(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializePubAck(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializePubRec(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializePubRel(PacketPtr pkt, std::vector<uint8_t>& buffer);
    static bool serializePubComp(PacketPtr pkt, std::vector<uint8_t>& buffer);

    static size_t calcPublishRemainingLength(const PublishPacket& pkt);

    static inline const DeserializeFunc decoders_[] = {
        nullptr,
        deserializeConnect,
        nullptr,
        deserializePublish,
        deserializePubAck,
        deserializePubRec,
        deserializePubRel,
        deserializePubComp,
    };

    static inline const SerializeFunc encoders_[] = {
        nullptr,
        nullptr,
        serializeConnack,
        serializePublish,
        serializePubAck,
        serializePubRec,
        serializePubRel,
        serializePubComp,
    };
};

} // namespace protocol
} // namespace jianm
