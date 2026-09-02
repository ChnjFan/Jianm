/*
 * File: /ClientContext.hpp
 * Project: broker
 * Created Date: 2026-08-23 16:03:05
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-31 12:09:10
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

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>

#include "jianm/model/Qos.hpp"
#include "jianm/model/Session.hpp"


namespace jianm {

namespace net { class Channel; }

namespace broker {

/**
 * @brief Will message
 * 
 * published when the connection is abnormally disconnected
 */
struct Will {
    std::string topic;
    std::string payload;
    Qos qos{Qos::AtMostOnce};
    bool retain{false};
    bool valid{false};  // Does CONNECT carry a will
};

/**
 * @brief Client‑side context
 * 
 * Connection + Session + QoS state machine, the hub between the connection layer and the protocol layer
 */
struct ClientContext {
    std::weak_ptr<jianm::net::Channel> channel;
    std::weak_ptr<Session> session;     // Held by SessionManager
    std::string client_id;
    bool connected = false;             // received CONNECT and reply CONNACK accept
    bool clean_disconnect = false;      // received DISCONNECT and not send will
    bool taken_over = false;            // Replaced by a new connection with the same client_id
    Will will;

    // Inbound QoS Status: This client acts as a publisher， key is Packet ID
    std::unordered_map<uint16_t, std::string> awaiting_puback;  // pid -> topic
    std::unordered_map<uint16_t, std::string> awaiting_pubrel;  // QoS2 pid -> topic

    // Outbound QoS status: This client acts as a subscriber
    // messages are delivered to it by the broker
    struct OutItem {
        Qos qos{Qos::AtMostOnce};
        bool pubrel_sent{false};
    };
    std::unordered_map<uint16_t, OutItem> out_inflight; // key is Packet ID
    uint16_t next_out_pid = 1;

    uint16_t nextPacketId() { return next_out_pid++; }
};

} // namespace broker
} // namespace jianm