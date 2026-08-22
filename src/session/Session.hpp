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

#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>

#include "protocol/ConnMessage.hpp"
#include "net/Channel.hpp"

namespace jianm {
namespace session {

enum class SessionState : uint8_t {
    CONNECTING = 0,
    CONNECTED = 1,
    DISCONNECTING = 2,
    ACTIVE = 3, // auth success, and sending msg
    CONNECT_PENDING = 4,
};

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session();
    ~Session();

    explicit Session(const std::string& clientID);

    jianm::protocol::ReasonCode connect(const std::shared_ptr<jianm::protocol::ConnMessage>& connMsg);
    void close();
    void connack(jianm::protocol::ConnAckReasonCode rc, uint8_t present);

    void bindChannel(std::weak_ptr<jianm::net::Channel> channel);
    std::shared_ptr<jianm::net::Channel> channel() { return channel_.lock(); }

    SessionState getState() const { return state_; }
    void setState(SessionState state) { state_ = state; }

    bool isCleanSession() const { return isCleanSession_; }

private:

    std::string clientID_;
    SessionState state_ = SessionState::CONNECTING;

    bool isCleanSession_ = true;
    uint16_t expiryInterval_ = 0;
    uint16_t keepalive_ = 0;

    int willQos_ = 0;
    bool willRetain_ = false;
    std::string willTopic_;
    std::string willMessage_;

    std::string username_;
    std::string password_;

    std::weak_ptr<jianm::net::Channel> channel_;
};

} // namespace session
} // namespace jianm

#endif

