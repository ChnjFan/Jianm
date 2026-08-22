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

#include <iostream>

#include "SessionMgr.hpp"
#include "protocol/MessageMgr.hpp"
#include "protocol/ConnMessage.hpp"
#include "common/ConfigMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"

using namespace jianm::session;

SessionMgr::~SessionMgr()
{
    stop();
}

void SessionMgr::start()
{
    if (running_.exchange(true)) {
        return;
    }
    workerThread_ = std::thread(&SessionMgr::workerLoop, this);
}

void SessionMgr::stop()
{
    if (!running_.exchange(false)) {
        return;
    }
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void SessionMgr::workerLoop()
{
    while (running_.load()) {
        handleRequest();
        retry();
    }
}

void SessionMgr::handleRequest()
{
    const auto request = jianm::protocol::MessageMgr::getInstance()->getRequest();
    if (!request) {
        return;
    }

    if (handlers_.find(request->getType()) == handlers_.end()) {
        return;
    }

    try
    {
        handlers_[request->getType()](request);
    }
    catch(const std::exception& e)
    {
        JM_LOG_WARN("request handle error: {}", e.what());
    }
}

void SessionMgr::createSession(std::shared_ptr<jianm::net::Channel> channel, const std::string &clientID)
{
    if (clientID.empty()) {
        return;
    }

    std::shared_ptr<Session> session;
    if (sessions_.find(clientID) != sessions_.end()) {
        //Specification requirement [MQTT‑3.1.4‑2]: 
        // If a client corresponding to the ClientId is already connected, 
        // the server MUST disconnect the existing connection (close the old channel) 
        // before accepting the new connection.
        session = sessions_[clientID];
    }
    else {
        session = std::make_shared<Session>(clientID);
        if (!session) {
            channel->close();
            throw std::runtime_error("std::make_shared<Session> error");
        }
    }

    // The old Channel will be released when binding a new Channel
    session->bindChannel(channel);
    channel->setConnected();
    sessions_.insert({clientID, session});
}

void SessionMgr::closeSession(const std::string &clientID)
{
    if (sessions_.find(clientID) == sessions_.end()) {
        return;
    }

    auto session = sessions_[clientID];

    auto channel = session->channel();
    if (channel) {
        channel->close();
    }

    if (session->isCleanSession()) {
        session->close();
        sessions_.erase(clientID);
    }
    else {
        // TODO: The session needs to be removed from sessions_ and added to the retained session list.
        // MQTT 3.1.1 does not have the Session Expiry Interval.
        session->setState(SessionState::CONNECT_PENDING);
    }
}

void SessionMgr::closeChannel(const std::string &clientID)
{
    if (sessions_.find(clientID) == sessions_.end()) {
        return;
    }

    auto session = sessions_[clientID];

    auto channel = session->channel();
    if (channel) {
        channel->close();
    }
}

SessionMgr::SessionMgr() {
    initHandlers();
}

void SessionMgr::initHandlers()
{
    registerHandler(jianm::protocol::MessageType::CONNECT, [this](std::shared_ptr<jianm::protocol::Message> request){
        return connectHandler(request);
    });
}

void SessionMgr::registerHandler(jianm::protocol::MessageType type, const RequestHandler &handler)
{
    if (handlers_.find(type) != handlers_.end()) {
        return;
    }
    handlers_.insert({type, handler});
}

void SessionMgr::retry()
{
    static auto last = Clock::now();
    auto now = Clock::now();
    if (jianm::common::is_timeout(last, now, 5s)) {
        last = Clock::now();
        keelalive(now);
    }
}

void jianm::session::SessionMgr::keelalive(TimePoint now)
{
    // Specification requirement [MQTT‑3.1.2.10]:
    // The server must receive any data packet from the client within 1.5 × Keep Alive time;
    // otherwise, it shall disconnect the connection.
    //
    // Iterator-safe traversal: closeSession erases from sessions_, so advance
    // the iterator before erasing to avoid invalidation.
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        auto& session = it->second;
        ++it;
        // TODO: Retain sessions will be using a other queue.
        if ((session->getState() == SessionState::CONNECTED
            || session->getState() == SessionState::ACTIVE)
             && session->getKeepAlive() > 0) {
            auto timeout = std::chrono::milliseconds(session->getKeepAlive() * 1500);
            if (jianm::common::is_timeout(session->getLastRecvTime(), now, timeout)) {
                JM_LOG_WARN("Session {} keepalive timeout, closing", session->getClientID());
                closeSession(session->getClientID());
            }
        }
    }
}

void SessionMgr::connectHandler(std::shared_ptr<jianm::protocol::Message> request)
{
    auto connMsg = std::dynamic_pointer_cast<jianm::protocol::ConnMessage>(request);
    auto& msg = connMsg->getMessage();

    std::string clientID = msg.payload.client_id;
    if (sessions_.find(clientID) == sessions_.end()) {
        throw std::runtime_error("Not found session from " + clientID);
    }

    auto session = sessions_[clientID];
    session->setLastRecvTime(Clock::now());
    // SessionState::CONNECTING, it means the session did not exist before
    uint8_t present = (msg.bits.clean_session == 0 
                        && session->getState() != SessionState::CONNECTING) ? 1 : 0;
    if (msg.bits.clean_session == 0 && session->getState() != SessionState::CONNECTING) {
        session->setState(SessionState::CONNECTING);
        // TODO: clean topic subscribe lists
    }

    if (msg.payload.client_id.length() > 23
        || !jianm::common::is_valid_utf8(msg.payload.client_id)) {
        // The Client ID must be UTF‑8 encoded
        session->connack(protocol::ConnAckReturnCode::REFUSED_IDENTIFIER_REJECTED, 0);
        closeSession(clientID);
        return;
    }

    if (msg.bits.username != 0 
        && (msg.payload.username.empty()
            || !jianm::common::is_valid_utf8(msg.payload.username))) {
        session->connack(protocol::ConnAckReturnCode::REFUSED_BAD_USERNAME_PASSWORD, 0);
        closeSession(clientID);
        return;
    }

    if (msg.bits.password != 0 && msg.payload.password.empty()) {
        session->connack(protocol::ConnAckReturnCode::REFUSED_BAD_USERNAME_PASSWORD, 0);
        closeSession(clientID);
        return;
    }

    // if allow_anonymous is false, we need to check username and password
    if (jianm::common::ConfigMgr::getInstance()["allow_anonymous"].empty()
        || jianm::common::ConfigMgr::getInstance()["allow_anonymous"] == "false") {
        if (msg.bits.username == 0
            || msg.bits.password == 0
            || !sessionAuthen(msg.payload.username, msg.payload.password)) {
            // [MQTT‑3.2.2‑4]: When the CONNACK return code is non‑zero
            //   Session Present MUST be set to 0
            session->connack(protocol::ConnAckReturnCode::REFUSED_BAD_USERNAME_PASSWORD, 0);
            closeSession(clientID);
            return;
        }
    }

    if (checkWillInvalid(msg)) {
        closeSession(clientID);
        return;
    }

    if (session->connect(connMsg) != jianm::protocol::ReturnCode::SUCCESS) {
        // if packet error, close channel 
        closeSession(clientID);
        return;
    }

    session->connack(protocol::ConnAckReturnCode::ACCEPTED, present);
}

bool SessionMgr::sessionAuthen(const std::string &username, const std::string &password)
{
    (void)username;
    (void)password;
    return true;
}

bool SessionMgr::checkWillInvalid(const jianm::protocol::ConnectMessage &msg)
{
    if (msg.bits.will == 1) {
        return msg.bits.will_qos > 2
            || msg.payload.will_topic.empty();
    }
    else {
        return msg.bits.will_qos != 0
            || msg.bits.will_retain != 0;
    }
    return false;
}
