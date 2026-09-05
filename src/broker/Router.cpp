/*
 * File: /Router.cpp
 * Project: broker
 * Created Date: 2026-08-24 22:10:35
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 22:14:36
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

#include "common/Logger.hpp"
#include "TopicTree.hpp"
#include "SessionManager.hpp"
#include "Outbox.hpp"

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
        if (!subscriber)
            return;

        auto session = subscriber->session.lock();
        auto sub_channel = subscriber->channel.lock();
        const Qos effectiveQos = static_cast<uint8_t>(m.qos) < static_cast<uint8_t>(msg.qos)
                                    ? m.qos : msg.qos;
        if (subscriber && subscriber->connected
            && sub_channel && !sub_channel->isClosing()) {
            deliver(subscriber, msg, m.qos, false);
        }
        else if (!sub_session->clean_session && effectiveQos > Qos::AtMostOnce) {
            services_.outbox.enqueue(sub_session, msg, effectiveQos);
        }
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
        subscriber->out_inflight[pub.packet_id] = {pub.qos, false, msg.topic, msg.payload, msg.source_client};
    }

    if (sub_channel->asyncSend(pkt)) {
        services_.delivered++;
    }
    else {
        subscriber->out_inflight.erase(pub.packet_id);
    }
}

void Router::resend(std::shared_ptr<ClientContext> subscriber, uint16_t packet_id)
{
    auto it = subscriber->out_inflight.find(packet_id);
    if (it == subscriber->out_inflight.end()) {
        JM_LOG_WARN("resend: packet {} not found in out_inflight for client {}", packet_id, subscriber->client_id);
        return;
    }
    auto channel = subscriber->channel.lock();
    if (!subscriber->connected || !channel || channel->isClosing()) {
        JM_LOG_WARN("resend: client {} not connected or channel closing", subscriber->client_id);
        return;
    }

    auto& item = it->second;
    auto pkt = std::make_shared<Packet>();
    if (item.qos == Qos::ExactlyOnce && item.pubrel_sent) {
        // Already sent PUBREL, waiting for PUBCOMP
        pkt->type = PacketType::Pubrel;
        auto& pubrel = pkt->body.emplace<AckPacket>();
        pubrel.packet_id = packet_id;
        channel->asyncSend(pkt);
    }
    else {
        // Specification requirement [MQTT‑3.3.1]:
        // Qos1 and Qos2 resend PUBLISH, DUP=1, retain=0
        pkt->type = PacketType::Publish;
        auto& pub = pkt->body.emplace<PublishPacket>();
        pub.topic = item.topic;
        pub.payload = item.payload;
        pub.qos = item.qos;
        pub.retain = false;
        pub.dup = true;
        pub.packet_id = packet_id;
        pub.source_client = item.source_client;
        if (!channel->asyncSend(pkt)) {
            JM_LOG_WARN("resend: failed to send packet {} to client {}", packet_id, subscriber->client_id);
        }
        else {
            item.sent_time = clock::now();
            item.retry_count++;
        }
    }
}
