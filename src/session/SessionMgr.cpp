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
        // TODO: check session state
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
    if (sessions_.find(clientID) != sessions_.end()) {
        return;
    }

    auto session = std::make_shared<Session>(clientID);
    if (!session) {
        throw std::runtime_error("std::make_shared<Session> error");
    }

    session->bindChannel(channel);
    channel->setConnected();
    if (clientID.empty()) {
        return;
    }
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
}

void jianm::session::SessionMgr::closeChannel(const std::string &clientID)
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

void SessionMgr::connectHandler(std::shared_ptr<jianm::protocol::Message> request)
{
    auto connMsg = std::dynamic_pointer_cast<jianm::protocol::ConnMessage>(request);
    auto& msg = connMsg->getMessage();

    std::string clientID = msg.payload.client_id;
    if (sessions_.find(clientID) == sessions_.end()) {
        throw std::runtime_error("Not found session from " + clientID);
    }

    auto session = sessions_[clientID];
    uint8_t present = session->getState() == SessionState::CONNECTING ? 0 : 1;

    // if allow_anonymous is false, we need to check username and password
    if (jianm::common::ConfigMgr::getInstance()["allow_anonymous"].empty()
        || jianm::common::ConfigMgr::getInstance()["allow_anonymous"] == "false") {
        if (msg.bits.username == 0
            || msg.bits.password == 0
            || !sessionAuthen(msg.payload.username, msg.payload.password)) {
            session->connack(protocol::ConnAckReasonCode::REFUSED_BAD_USERNAME_PASSWORD, present);
            return;
        }
    }

    if (session->connect(connMsg) != jianm::protocol::ReturnCode::SUCCESS) {
        // if packet error, close channel 
        closeSession(clientID);
    }

    session->connack(protocol::ConnAckReasonCode::ACCEPTED, present);
}

bool SessionMgr::sessionAuthen(const std::string &username, const std::string &password)
{
    (void)username;
    (void)password;
    return true;
}
