/*
 * File: /Outbox.cpp
 * Project: broker
 * Created Date: 2026-09-05 21:45:39
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 22:00:20
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

#include "Outbox.hpp"

using namespace jianm::broker;

void Outbox::enqueue(const SessionPtr &session, const Message &msg, Qos granted_qos, bool retained)
{
    // QoS 0 discards messages for offline clients
    if (granted_qos == Qos::AtMostOnce)
        return;
    
    auto& q = queues_[session.get()];
    // Discard the oldest message when the queue is full
    // head‑drop new‑message‑preferring policy
    if (q.size() >= MaxQueueSize)
        q.pop_front();
    q.push_back({msg, granted_qos, retained});
}

void Outbox::clear(const SessionPtr &session)
{
    queues_.erase(session.get());
}

size_t Outbox::size(const SessionPtr &session) const
{
    const auto it = queues_.find(session.get());
    return it == queues_.end() ? 0 : it->second.size();
}

size_t Outbox::totalSize() const
{
    size_t total = 0;
    for (const auto& [_, q] : queues_)
        total += q.size();
    return total;
}

