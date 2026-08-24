/*
 * File: /SessionManager.hpp
 * Project: broker
 * Created Date: 2026-08-23 10:24:26
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 12:34:22
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

#include <atomic>
#include <thread>
#include <unordered_map>

#include "jianm/model/Session.hpp"

#include "common/Singleton.hpp"
#include "net/Channel.hpp"

#include "ClientContext.hpp"

namespace jianm {
namespace broker {

class SessionManager
{
public:
    SessionManager();
    ~SessionManager();

    void start();
    void stop();

    std::shared_ptr<ClientContext> addChannel(jianm::net::ChannelPtr channel);
    std::shared_ptr<ClientContext> byChannel(jianm::net::ChannelPtr channel);

    void bindId(const std::string& client_id, std::shared_ptr<ClientContext> ctx);
    std::shared_ptr<ClientContext> byId(const std::string& clientId);

    void removeChannel(const jianm::net::ChannelPtr& channel);

    bool sessionExists(const std::string& client_id) const;
    std::shared_ptr<jianm::Session> getSession(const std::string& client_id, bool clean);

    /// Number of active channel connections (by_channel_ size)
    size_t connectionCount() const;
    /// Number of stored sessions (sessions_ size)
    size_t sessionCount() const;

private:
    void dropSession(const std::shared_ptr<jianm::Session>& session);

    std::unordered_map<std::string, std::shared_ptr<jianm::Session>> sessions_;
    std::unordered_map<jianm::net::Channel*, std::shared_ptr<ClientContext>> by_channel_;
    std::unordered_map<std::string, std::shared_ptr<ClientContext>> by_id_;
};

} // namespace broker
} // namespace jianm
