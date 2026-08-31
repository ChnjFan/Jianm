/*
 * File: /Router.cpp
 * Project: broker
 * Created Date: 2026-08-24 22:10:35
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-25 10:05:29
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

#include "Router.hpp"

#include "jianm/model/Packet.hpp"

#include "TopicTree.hpp"
#include "SessionManager.hpp"

using namespace jianm::broker;

Router::Router(BrokerServices &service) : services_(service)
{
}

void Router::route(const Message &msg)
{
    for (const auto& m : services_.topics.match(msg.topic)) {
        auto sub_session = m.session.lock();
        if (!sub_session) continue;
        auto subscriber = services_.sessions.byId(sub_session->client_id);
        if (!subscriber || !subscriber->connected || !subscriber->channel.lock())
            continue;
        auto sub_channel = subscriber->channel.lock();
        deliver(subscriber, msg, m.qos, false);
    }
}

void Router::deliver(std::shared_ptr<ClientContext> subscriber, const Message &msg, Qos granted_qos, bool as_retained)
{
    auto sub_channel = subscriber->channel.lock();
    if (!sub_channel || sub_channel->isClosing())
        return;
    
    auto pkt = std::make_shared<Packet>();
    pkt->type = PacketType::Publish;
    auto& pub = pkt->body.emplace<PublishPacket>();
    pub.topic = msg.topic;
    pub.payload = msg.payload;
    pub.qos = static_cast<uint8_t>(granted_qos)
                 < static_cast<uint8_t>(msg.qos) ? granted_qos : msg.qos;
    pub.retain = as_retained;
    pub.source_client = msg.source_client;

    if (pub.qos != Qos::AtMostOnce) {
        pub.packet_id = subscriber->nextPacketId();
        subscriber->out_inflight[pub.packet_id] = {pub.qos, false};
    }

    if (sub_channel->asyncSend(pkt)) {
        services_.delivered++;
    }
    else {
        subscriber->out_inflight.erase(pub.packet_id);
    }
}
