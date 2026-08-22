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

#include "Session.hpp"
#include "common/Utils.hpp"
#include "common/ConfigMgr.hpp"
#include "protocol/Message.hpp"

using namespace jianm::session;

Session::Session()
{
}

Session::~Session()
{
}

Session::Session(const std::string &clientID)
    : clientID_(clientID), state_(SessionState::CONNECTING)
{
}

jianm::protocol::ReturnCode Session::connect(const std::shared_ptr<jianm::protocol::ConnMessage> &connMsg)
{
    jianm::protocol::ReturnCode rc = jianm::protocol::ReturnCode::SUCCESS;
    auto& request = connMsg->getMessage();

    if (clientID_ != request.payload.client_id) {
        return jianm::protocol::ReturnCode::PROTOCOL_ERROR;
    }

    isCleanSession_ = (request.bits.clean_session == 1);
    if (isCleanSession_) {
        expiryInterval_ = UINT16_MAX;
    }

    keepalive_ = request.payload.keep_alive;

    if (request.bits.will == 1) {
        willQos_ = request.bits.will_qos;
        willRetain_ = (request.bits.will_retain == 1);
        willTopic_ = request.payload.will_topic;
        willMessage_ = request.payload.will_message;
    }

    username_ = request.payload.username;
    password_ = request.payload.password;

    state_ = SessionState::CONNECTED;

    return rc;
}

void Session::close()
{
}

void jianm::session::Session::connack(jianm::protocol::ConnAckReasonCode rc, uint8_t present)
{
    jianm::protocol::ConnAckMessage msg = jianm::protocol::ConnAckMessage()
        .setSessionPresent(present)
        .setReturnCode(static_cast<uint8_t>(rc));

    std::vector<uint8_t> buffer;
    msg.serialize(buffer);
    auto channel = channel_.lock();
    if (channel) {
        channel->asyncSend(std::move(buffer));
        setLastSendTime(Clock::now());
    }
}

void Session::bindChannel(std::weak_ptr<jianm::net::Channel> channel)
{
    auto oldChannel = channel_.lock();
    if (oldChannel) {
        oldChannel->close();
    }

    channel_ = channel;
}

