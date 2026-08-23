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

#ifndef SESSION_MGR_HPP
#define SESSION_MGR_HPP

#include <atomic>
#include <thread>
#include <unordered_map>

#include "Session.hpp"
#include "common/Singleton.hpp"
#include "protocol/mqtt.h"
#include "protocol/MessageMgr.hpp"
#include "net/Channel.hpp"


namespace jianm {
namespace broker {

using RequestHandler = std::function<void(std::shared_ptr<jianm::protocol::Message>)>;

class SessionMgr : public common::Singleton<SessionMgr>
{
public:
    ~SessionMgr();

    void start();
    void stop();

    void handleRequest();

    void createSession(std::shared_ptr<jianm::net::Channel> channel, const std::string& clientID);
    void closeSession(const std::string& clientID, jianm::protocol::ReturnCode reason);
    void closeChannel(const std::string& clientID, jianm::protocol::ReturnCode reason);

private:
    friend class Singleton<SessionMgr>;
    SessionMgr();

    void initHandlers();
    void registerHandler(jianm::protocol::MessageType type, const RequestHandler& handler);

    void retry();
    void keelalive(TimePoint now);

    void connectHandler(std::shared_ptr<jianm::protocol::Message> request);
    bool sessionAuthen(const std::string& username, const std::string& password);
    bool checkWillInvalid(const jianm::protocol::ConnectMessage& msg);

    void workerLoop();

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;

    std::unordered_map<jianm::protocol::MessageType, RequestHandler> handlers_;

    std::thread workerThread_;
    std::atomic<bool> running_{false};
};

} // namespace broker
} // namespace jianm

#endif // SESSION_MGR_HPP