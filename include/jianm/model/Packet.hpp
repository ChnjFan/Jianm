/*
 * File: /Packet.hpp
 * Project: model
 * Created Date: 2026-08-23 15:31:18
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 12:53:21
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

#include "jianm/model/Qos.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace jianm {
namespace broker {

#define JM_MQTT_3_1_1_VERSION_LEVEL     (4)

constexpr uint8_t MQTT_CONNACK_BYTE = 0x20;

/// @brief Message Type (MQTT 3.1.1 Control Packet)
enum class PacketType : uint8_t {
    Connect = 1,
    Connack = 2,
    Publish = 3,
    Puback = 4,
    Pubrec = 5,
    Pubrel = 6,
    Pubcomp = 7,
    Subscribe = 8,
    Suback = 9,
    Unsubscribe = 10,
    Unsuback = 11,
    Pingreq = 12,
    Pingresp = 13,
    Disconnect = 14,
};

/// @brief Empty message packet
struct EmptyPacket {};

struct ConnectFlags {
    uint8_t reserved : 1;
    uint8_t clean_session : 1;
    uint8_t will : 1;
    uint8_t will_qos : 2;
    uint8_t will_retain : 1;
    uint8_t password : 1;
    uint8_t username : 1;
};

/// @brief CONNECT message packet
struct ConnectPacket {
    uint8_t level{4};
    std::string protocol;
    std::string client_id;
    std::string username;
    std::string password;
    std::string will_topic;
    std::string will_payload;
    Qos will_qos{Qos::AtMostOnce};
    bool will_retain{false};
    bool has_will{false};
    bool clean_session{true};
    uint16_t keepalive{0};
    union 
    {
        uint8_t byte;
        ConnectFlags bits;
    };
};

enum class ConnackReturnCode : uint8_t {
    accepted = 0,
    bad_protocol_version = 1,
    id_rejected = 2,
    server_unavailable = 3,
    bad_username_password = 4,
    not_authorized = 5,
};

/// @brief CONNACK message packet
struct ConnackPacket {
    bool session_present{false};
    ConnackReturnCode return_code{ConnackReturnCode::accepted};
};

/// @brief PUBLISH message packet
struct PublishPacket {
    std::string topic;
    std::string payload;
    Qos qos{Qos::AtMostOnce};
    bool retain{false};
    bool dup{false};
    uint16_t packet_id{0};
    std::string source_client;  // For audit/plugin purposes, not a protocol field
};

struct SubscribeEntry {
    std::string filter;
    Qos qos{Qos::AtMostOnce};
};

struct SubscribePacket {
    uint16_t packet_id{0};
    std::vector<SubscribeEntry> entries;
};

struct SubackPacket {
    uint16_t packet_id{0};
    std::vector<uint8_t> granted;  // 0/1/2 = Granted QoS, 0x80 = Failure
};

struct UnsubscribePacket {
    uint16_t packet_id{0};
    std::vector<std::string> topics;
};

/// @brief PUBACK / PUBREC / PUBREL / PUBCOMP / UNSUBACK
struct AckPacket {
    uint16_t packet_id{0};
};

/// @brief Unified Message Model: Type + Specific Payload
struct Packet {
    PacketType type{PacketType::Pingreq};
    std::variant<ConnectPacket, ConnackPacket, PublishPacket, SubscribePacket, SubackPacket,
                 UnsubscribePacket, AckPacket, EmptyPacket> body;
};

union Header {
    uint8_t byte;
    struct {
        uint8_t retain : 1;
        unsigned int qos : 2;
        uint8_t dup : 1;
        unsigned int type : 4;
    } bits;
};

}  // namespace broker
}  // namespace mqtt