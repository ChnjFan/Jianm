/*
 * File: /Outbox.hpp
 * Project: broker
 * Created Date: 2026-09-05 21:21:32
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 22:00:26
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

#include <deque>
#include <unordered_map>

#include "jianm/model/Message.hpp"
#include "jianm/model/Qos.hpp"
#include "jianm/model/Session.hpp"

namespace jianm {
namespace broker {

using SessionPtr = std::shared_ptr<Session>;

struct OutboxMessage
{
    Message msg;
    Qos granted_qos;
    bool retained;
};

/// @brief Offline message queue: stored by Session, drained upon client reconnection
class Outbox
{
public:
    static constexpr size_t MaxQueueSize = 1000;
    
    /**
     * @brief Cache a message for offline subscribers
     * 
     * @param session 
     * @param msg 
     * @param granted_qos 
     * @param retained is retained msg
     * 
     * Specification requirement [MQTT‑3.1.2‑5]:
     * After the disconnection of a Session that had CleanSession set to 0,
     * the Server MUST store further QoS 1 and QoS 2 messages that match any
     * subscriptions that the client had at the time of disconnection as part
     * of the Session state. The Server MAY also store QoS 0 messages.
     */
    void enqueue(const SessionPtr& session, const Message& msg, Qos granted_qos, bool retained = false);
    
    template <typename DeliverFn>
    size_t drain(const SessionPtr& session, DeliverFn deliver) {
        const auto it = queues_.find(session.get());
        if (it == queues_.end()) return 0;
        size_t count = 0;
        for (const auto&[msg, granted_qos, retained] : it->second) {
            deliver(msg, granted_qos, retained);
            ++count;
        }
        queues_.erase(it);
        return count;
    }

    void clear(const SessionPtr& session);
    
    size_t size(const SessionPtr& session) const;

    size_t totalSize() const;
    
private:
    std::unordered_map<Session*, std::deque<OutboxMessage>> queues_;
};


} // namespace broker
} // namespace jianm

