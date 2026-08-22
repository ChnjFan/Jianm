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

#ifndef MQTT_H
#define MQTT_H

/*
 * File: protocol/mqtt.h
 *
 * The header file contains values defined in the MQTT protocol.
 */

#include <iostream>

namespace jianm {
namespace protocol {

constexpr uint8_t MQTT_PROTOCOL_V31 = 4;
constexpr int MQTT_HPPEADER_LEN = 2;

// Stub bytes used in the fixed header of response messages
constexpr uint8_t MQTT_CONNECT_BYTE = 0x10;
constexpr uint8_t MQTT_CONNACK_BYTE = 0x20;
constexpr uint8_t MQTT_PUBLISH_BYTE = 0x30;
constexpr uint8_t MQTT_PUBACK_BYTE = 0x40;
constexpr uint8_t MQTT_PUBREC_BYTE = 0x50;
constexpr uint8_t MQTT_PUBREL_BYTE = 0x60;
constexpr uint8_t MQTT_PUBCOMP_BYTE = 0x70;
constexpr uint8_t MQTT_SUBACK_BYTE = 0x90;
constexpr uint8_t MQTT_UNSUBACK_BYTE = 0xB0;
constexpr uint8_t MQTT_PINGRESP_BYTE = 0xD0;

enum class ReturnCode : uint8_t {
    SUCCESS = 0,
    NORMAL_DISCONNECTION = 0,
    GRANTED_QOS0 = 0,
    GRANTED_QOS1 = 1,
    GRANTED_QOS2 = 2,
    DISCONNECT_WITH_WILL_MSG = 4,
    NO_MATCHING_SUBSCRIBERS = 16,
    NO_SUBSCRIPTION_EXISTED = 17,

    UNSPECIFIED = 128,                  // unkown err
    MALFORMED_PACKET = 129,             // CONNACK, DISCONNECT, Invalid packet
    PROTOCOL_ERROR = 130,               // DISCONNECT
    BAD_USERNAME_OR_PASSWORD = 134,     // CONNACK
    NOT_AUTHORIZED = 135,
    SERVER_UNAVAILABLE = 136,
    SERVER_BUSY = 137,
    BANNED = 138,
    BAD_AUTHENTICATION_METHOD = 140,
    TOPIC_NAME_INVALID = 144,
    PACKET_IDENTIFIER_IN_USE = 145,
    PACKET_IDENTIFIER_NOT_FOUND = 146,
    RECEIVE_MAXIMUM_EXCEEDED = 147,
    TOPIC_ALIAS_INVALID = 148,
    PACKET_TOO_LARGE = 149,
    MESSAGE_RATE_TOO_HPPIGH = 150,
    QUOTA_EXCEEDED = 151,
    ADMINISTRATIVE_ACTION = 152,
    PAYLOAD_FORMAT_INVALID = 153
};

enum class MessageType : uint8_t {
    CONNECT = 1,
    CONNACK = 2,
    PUBLISH = 3,
    PUBACK = 4,
    PUBREC = 5,
    PUBREL = 6,
    PUBCOMP = 7,
    SUBSCRIBE = 8,
    SUBACK = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK = 11,
    PINGREQ = 12,
    PINGRESP = 13,
    DISCONNECT = 14
};

enum class QoSLevel : uint8_t {
    AT_MOST_ONCE = 0,
    AT_LEAST_ONCE = 1,
    EXACTLY_ONCE = 2
};

// use a little-endian bitfield to represent the fixed header of MQTT messages
union Header {
    uint8_t byte;
    struct {
        uint8_t retain : 1;
        unsigned int qos : 2;
        uint8_t dup : 1;
        unsigned int type : 4;
    } bits;
};

} // namespace protocol
} // namespace jianm

#endif // MQTT_HPP