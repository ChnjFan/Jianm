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

#include "MessageMgr.hpp"
#include "session/SessionMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"

using namespace jianm::protocol;

MessageMgr::~MessageMgr()
{
    cond_.notify_all();
}

void MessageMgr::messageHandle(std::shared_ptr<net::Channel> channel, const std::vector<uint8_t> &buffer)
{
    if (buffer.empty()) {
        return;
    }
    Header header = { .byte = buffer[0] };
    std::shared_ptr<Message> msg = MessageFactory::createMessage(
            static_cast<MessageType>(header.bits.type));
    if (!msg) {
        JM_LOG_ERROR("Create message type={} error", static_cast<int>(header.bits.type));
        return;
    }
    if (msg->deserialize(buffer) != ReturnCode::SUCCESS) {    // Invalid packet will close channel
        channel->close();
        return;
    }

    JM_LOG_TRACE("Processing message type={} ({})",
        static_cast<int>(msg->getType()),
        messageTypeName(msg->getType()));

    if (msg->getType() == MessageType::CONNECT) {
        // check CONNACK packet valid
        ConnectMessage connMsg = std::dynamic_pointer_cast<ConnMessage>(msg)->getMessage();

        JM_LOG_TRACE("Received CONNECT: client_id=\"{}\" keep_alive={} clean_session={} "
             "will={} will_qos={} will_retain={} username={} password={}",
            connMsg.payload.client_id,
            connMsg.payload.keep_alive,
            static_cast<int>(connMsg.bits.clean_session),
            static_cast<int>(connMsg.bits.will),
            static_cast<int>(connMsg.bits.will_qos),
            static_cast<int>(connMsg.bits.will_retain),
            connMsg.payload.username.empty() ? "<none>" : connMsg.payload.username,
            connMsg.payload.password.empty() ? "<none>" : "***");

        // A CONNECT packet MUST be sent from the client to the server only once over a network connection.
        if (channel->isConnected()) {
            JM_LOG_INFO("Receive double CONNECT from {}, disconnecting session", connMsg.payload.client_id);
            session::SessionMgr::getInstance()->closeSession(connMsg.payload.client_id);
            return;
        }

        if (connMsg.payload.client_id.empty() && connMsg.bits.clean_session != 0) {
            common::generate_client_id(connMsg.payload.client_id);
        }
        else if (connMsg.payload.client_id.empty()) {
            // There have no client id, we don't know which session
            connack(channel, ConnAckReasonCode::REFUSED_IDENTIFIER_REJECTED);
            return;
        }

        if (connMsg.bits.clean_session != 0) {
            // Close old session, need send will message
            session::SessionMgr::getInstance()->closeSession(connMsg.payload.client_id);
        }
        session::SessionMgr::getInstance()->createSession(channel, connMsg.payload.client_id);
    }

    {
        std::lock_guard<std::mutex> lock(mtx_);
        requests_.push(msg);
        cond_.notify_one();
    }
    
}

std::shared_ptr<Message> MessageMgr::getRequest(const int wait)
{
    const int wait_time = wait < 1000 ? 1000 : wait;
    std::unique_lock<std::mutex> lock(mtx_);
    cond_.wait_for(lock, std::chrono::milliseconds(wait_time), [this]{
        return !requests_.empty();
    });
    if (requests_.empty()) {
        return nullptr;
    }
    auto msg = requests_.front();
    requests_.pop();
    return msg;
}

void MessageMgr::connack(const std::shared_ptr<net::Channel> &channel, ConnAckReasonCode rc, uint8_t present)
{
    ConnAckMessage msg = ConnAckMessage()
        .setSessionPresent(present)
        .setReturnCode(static_cast<uint8_t>(rc));

    std::vector<uint8_t> buffer;
    msg.serialize(buffer);
    channel->asyncSend(std::move(buffer));
}

std::shared_ptr<Message> MessageFactory::createMessage(MessageType type)
{
    switch (type)
    {
    case MessageType::CONNECT:
        return std::make_shared<ConnMessage>();
        break;
    
    default:
        break;
    }
    return nullptr;
}
