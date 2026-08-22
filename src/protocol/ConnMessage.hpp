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

#ifndef CONN_MESSAGE_HPP
#define CONN_MESSAGE_HPP

#include "Message.hpp"

namespace jianm {
namespace protocol {

struct Flags {
    uint8_t reserved : 1;
    uint8_t clean_session : 1;
    uint8_t will : 1;
    uint8_t will_qos : 2;
    uint8_t will_retain : 1;
    uint8_t password : 1;
    uint8_t username : 1;
};

struct ConnectMessage {
    Header header;
    union {
        uint8_t byte;
        Flags bits;
    };
    struct Payload {
        uint16_t keep_alive;
        std::string client_id;
        std::string will_topic;
        std::string will_message;
        std::string username;
        std::string password;
    } payload;
};

struct AckFlags {
    uint8_t session_present : 1;
    uint8_t reserved : 7;
};

enum class ConnAckReturnCode : uint8_t {
    ACCEPTED = 0,
    REFUSED_PROTOCOL_VERSION = 1,
    REFUSED_IDENTIFIER_REJECTED = 2,
    REFUSED_SERVER_UNAVAILABLE = 3,
    REFUSED_BAD_USERNAME_PASSWORD = 4,
    REFUSED_NOT_AUTHORIZED = 5,
};

struct ConnectAckMessage {
    Header header;
    union {
        unsigned char byte;
        AckFlags bits;
    };
    uint8_t return_code;
};

static const ConnectMessage DEFAULT_CONNECT_MESSAGE = {
    .header = {.byte = MQTT_CONNECT_BYTE},
    .byte = 0,
    .payload = {
        .keep_alive = 60,
        .client_id = "",
        .will_topic = "",
        .will_message = "",
        .username = "",
        .password = ""
    }
};

static const ConnectAckMessage DEFAULT_CONNACK_MESSAGE = {
    .header = {.byte = MQTT_CONNACK_BYTE},
    .byte = 0,
    .return_code = 0
};

class ConnMessage : public Message {
public:
    ConnMessage() : message_(DEFAULT_CONNECT_MESSAGE) {}
    explicit ConnMessage(const ConnectMessage& message) : message_(message) {};
    virtual ~ConnMessage() = default;

    MessageType getType() const override { return MessageType::CONNECT; }
    const ConnectMessage& getMessage() const { return message_; }

    ReturnCode serialize(std::vector<uint8_t>& buffer) const override;
    ReturnCode deserialize(const std::vector<uint8_t>& buffer) override;

    ReturnCode checkPacket() const override;

private:
    ConnectMessage message_;
};

class ConnAckMessage : public Message {
public:
    ConnAckMessage() : message_(DEFAULT_CONNACK_MESSAGE) {}
    explicit ConnAckMessage(const ConnectAckMessage& message) : message_(message) {};
    virtual ~ConnAckMessage() = default;

    MessageType getType() const override { return MessageType::CONNACK; }

    ReturnCode serialize(std::vector<uint8_t>& buffer) const override;
    ReturnCode deserialize(const std::vector<uint8_t>& buffer) override;

    ReturnCode checkPacket() const override;

    ConnAckMessage& setSessionPresent(int present) {
        message_.bits.session_present = present;
        return *this;
    }
    ConnAckMessage& setReturnCode(uint8_t rc) {
        message_.return_code = rc;
        return *this;
    }

private:
    ConnectAckMessage message_;
};

} // namespace protocol
} // namespace jianm

#endif // CONN_MESSAGE_HPP