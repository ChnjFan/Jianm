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
#include "broker/SessionMgr.hpp"
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
        JM_LOG_DEBUG("Packet type={} is not support", static_cast<int>(header.bits.type));
        channel->close();
        return;
    }
    ReturnCode rc = msg->deserialize(buffer);
    if (rc != ReturnCode::SUCCESS) {
        if (msg->getType() == MessageType::CONNECT
            && rc == ReturnCode::PROTOCOL_VERSION_NOT_SUPPORT) {
            // Specification requirement [MQTT‑3.1.2‑2]: 
            // If the protocol version is not supported, a CONNACK (return code = 0x01) 
            // must be sent first, and then the connection shall be closed.
            connack(channel, ConnAckReturnCode::REFUSED_PROTOCOL_VERSION);
        }
        channel->close();
        return;
    }

    rc = msg->checkPacket();
    if (rc != ReturnCode::SUCCESS) {
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

        if (channel->isConnected()) {
            // Specification requirement [MQTT‑3.1.0‑2]:
            // Receiving a second CONNECT on the same connection shall be treated as a protocol violation,
            // and the network connection must be closed.
            JM_LOG_INFO("Receive double CONNECT from {}, disconnecting session", connMsg.payload.client_id);
            broker::SessionMgr::getInstance()->closeSession(connMsg.payload.client_id,
                     ReturnCode::DISCONNECT_WITH_WILL_MSG);
            channel->close();
            return;
        }

        if (connMsg.payload.client_id.empty() && connMsg.bits.clean_session != 0) {
            common::generate_client_id(connMsg.payload.client_id);
        }
        else if (connMsg.payload.client_id.empty()) {
            // There have no client id, we don't know which session
            connack(channel, ConnAckReturnCode::REFUSED_IDENTIFIER_REJECTED);
            return;
        }

        if (connMsg.bits.clean_session != 0) {
            // Close old session, need send will message
            broker::SessionMgr::getInstance()->closeSession(connMsg.payload.client_id, ReturnCode::DISCONNECT_WITH_WILL_MSG);
        }
        broker::SessionMgr::getInstance()->createSession(channel, connMsg.payload.client_id);
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

void MessageMgr::connack(const std::shared_ptr<net::Channel> &channel, ConnAckReturnCode rc, uint8_t present)
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
