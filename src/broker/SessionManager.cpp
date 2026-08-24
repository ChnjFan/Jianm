/*
 * File: /SessionManager.cpp
 * Project: broker
 * Created Date: 2026-08-23 10:24:35
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 12:34:48
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


#include <iostream>

#include "SessionManager.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"
#include "ClientContext.hpp"

using namespace jianm::broker;

SessionManager::SessionManager()
{
}

SessionManager::~SessionManager()
{
    stop();
}

void SessionManager::start()
{
}

void SessionManager::stop()
{
}

std::shared_ptr<ClientContext> SessionManager::addChannel(jianm::net::ChannelPtr channel)
{
    auto ctx = std::make_shared<ClientContext>();
    ctx->channel = channel;
    by_channel_[channel.get()] = ctx;
    return ctx;
}

std::shared_ptr<ClientContext> SessionManager::byChannel(jianm::net::ChannelPtr channel)
{
    auto it = by_channel_.find(channel.get());
    return it == by_channel_.end() ? nullptr : it->second;
}

void SessionManager::bindId(const std::string &client_id, std::shared_ptr<ClientContext> ctx)
{
    by_id_[client_id] = ctx;
}

std::shared_ptr<ClientContext> SessionManager::byId(const std::string &clientId)
{
    auto it = by_id_.find(clientId);
    return it == by_id_.end() ? nullptr : it->second;
}

void SessionManager::removeChannel(const jianm::net::ChannelPtr &channel)
{
    const auto it = by_channel_.find(channel.get());
    if (it == by_channel_.end())
        return;
    
    const auto ctx = it->second;
    if (!ctx->client_id.empty()) {
        const auto jt = by_id_.find(ctx->client_id);
        if (jt != by_id_.end() && jt->second == ctx) {
            by_id_.erase(jt);
        }
    }

    // Sessions with clean_session are destroyed upon disconnection;
    // hijacked sessions are reserved for new channels
    const auto session = ctx->session.lock();
    if (session && !ctx->taken_over && session->clean_session) {
        dropSession(session);
        ctx->session.reset();
    }
    by_channel_.erase(it);
}

bool SessionManager::sessionExists(const std::string &client_id) const
{
    return sessions_.count(client_id) > 0;
}

size_t SessionManager::connectionCount() const
{
    return by_channel_.size();
}

size_t SessionManager::sessionCount() const
{
    return sessions_.size();
}

std::shared_ptr<jianm::Session> SessionManager::getSession(const std::string &client_id, bool clean)
{
    if (clean) {
        auto it = sessions_.find(client_id);
        if (it != sessions_.end()) {
            dropSession(it->second);
        }
        auto session = std::make_shared<jianm::Session>();
        session->client_id = client_id;
        session->clean_session = true;
        sessions_.emplace(client_id, session);
        return session;
    }

    auto it = sessions_.find(client_id);
    if (it != sessions_.end()) {
        return it->second;
    }

    auto session = std::make_shared<jianm::Session>();
    session->client_id = client_id;
    session->clean_session = false;
    sessions_.emplace(client_id, session);
    return session;
}

void SessionManager::dropSession(const std::shared_ptr<jianm::Session> &session)
{
    for (const auto& sub : session->subscriptions) {
        // TODO: Clear the session subscription list
        (void)sub;
    }
    sessions_.erase(session->client_id);
}
