/*
 * File: /Handlers.cpp
 * Project: broker
 * Created Date: 2026-08-23 16:33:49
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-31 09:07:46
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

#include "Handlers.hpp"

#include <stdexcept>
#include <string>
#include <variant>

#include "jianm/model/Topic.hpp"
#include "jianm/model/Message.hpp"

#include "plugin/HookRegistry.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"
#include "net/Channel.hpp"
#include "Services.hpp"
#include "SessionManager.hpp"
#include "Router.hpp"

using namespace jianm::broker;

namespace jianm {

static void connAck(std::shared_ptr<ClientContext> &client, PacketType type, uint16_t packet_id)
{
    auto pkt = std::make_shared<Packet>();
    pkt->type = type;
    auto& ack = pkt->body.emplace<AckPacket>();
    ack.packet_id = packet_id;
    auto channel = client->channel.lock();
    if (channel) {
        channel->asyncSend(pkt);
    }
}

}
void ConnectHandler::handle(BrokerServices &service, std::shared_ptr<ClientContext> &client,
     const std::shared_ptr<Packet> &pkt)
{
    if (client->connected) {
        // Specification requirement [MQTT‑3.1.0‑2]:
        // Receiving a second CONNECT on the same connection shall be treated as a protocol violation,
        // and the network connection must be closed.
        throw std::runtime_error("duplicate CONNECT");
    }

    const auto& cp = std::get<ConnectPacket>(pkt->body);
    if (cp.protocol != "MQTT") {
        // Specification requirement [MQTT-3.1.2.1]:
        // The Protocol Name is the UTF-8 encoded string "MQTT", capitalized as shown.
        // If the protocol name is incorrect, the Server MAY disconnect the Client.
        throw std::runtime_error("bad CONNECT protocol name");
    }

    ConnackReturnCode rc = ConnackReturnCode::accepted;
    jianm::common::Defer reject([&rc, &client]{
        if (rc == ConnackReturnCode::accepted) return;
        auto reject_out = std::make_shared<Packet>();
        reject_out->type = PacketType::Connack;
        // Specification requirement [MQTT-3.2.2.2]:
        // If the return code is non-zero, Session Present MUST be set to 0.
        auto& cp = reject_out->body.emplace<ConnackPacket>();
        cp.session_present = false;
        cp.return_code = rc;
        auto channel = client->channel.lock();
        if (channel) {
            channel->asyncSend(reject_out);
            channel->requestClose("CONNECT rejected");
        }
    });

    // Specification requirement [MQTT-3.1.2-2]:
    // If the Protocol Level is not supported, respond with CONNACK return code 0x01.
    if (cp.level != JM_MQTT_3_1_1_VERSION_LEVEL) {
        rc = ConnackReturnCode::bad_protocol_version;
        return;
    }

    std::string cid = cp.client_id;
    if (cid.empty()) {
        if (!cp.clean_session) {
            // Specification requirement [MQTT‑3.1.3‑8]:
            // If the Client supplies a zero‑byte ClientId with CleanSession set to 0,
            // the Server MUST respond to the CONNECT Packet with a CONNACK return code 0x02 
            // (Identifier rejected) and then close the Network Connection.
            rc = ConnackReturnCode::id_rejected;
            return;
        }
    }

    // TODO: authen CONNECT

    // Session Takeover: A new connection with the same ClientID kicks out the old connection
    // the old connection will not publish will messages and the session will not be destroyed
    if (auto old = service.sessions.byId(cid); old && old != client) {
        old->taken_over = true;
        old->session.reset();
        auto channel = old->channel.lock();
        if (channel) channel->requestClose("session takeover");
        old->channel.reset();
    }

    const bool existed = service.sessions.sessionExists(cid);
    client->session = service.sessions.getSession(cid, cp.clean_session);
    client->client_id = cid;
    client->connected = true;
    client->clean_disconnect = false;
    client->taken_over = false;
    client->will = Will{cp.will_topic, cp.will_payload, cp.will_qos, cp.will_retain, cp.has_will};

    auto channel = client->channel.lock();
    if (!channel) {
        throw std::runtime_error("channel not bind in ClientContext");
    }
    
    if (cp.keepalive > 0) {
        channel->setKeepalive(cp.keepalive);
    }
    service.sessions.bindId(cid, client);

    auto out = std::make_shared<Packet>();
    out->type = PacketType::Connack;
    auto& out_cp = out->body.emplace<ConnackPacket>();
    out_cp.session_present = existed;
    out_cp.return_code = ConnackReturnCode::accepted;
    channel->asyncSend(out);

    service.hooks.onClientConnected(cid, cp.username);
    JM_LOG_INFO("client connected: {} from {} {}", cid, channel->getPeer(),
                 cp.clean_session ? "(clean session)" : "(persistent session)");
}

void PublishHandler::handle(BrokerServices &service, std::shared_ptr<ClientContext> &client,
     const std::shared_ptr<Packet> &pkt)
{
    auto& pub = std::get<PublishPacket>(pkt->body);

    if (isTopicNameInvalid(pub.topic)) {
        throw std::runtime_error("invlaid PUBLISH topic");
    }

    // The source client for forwarding messages shall write the client ID 
    // of the client that received the message.
    pub.source_client = client->client_id;
    service.received++;

    // QoS Semantics: DUP retransmission deduplication
    // If the packet‑id has been received, only an ACK is returned without repeated routing
    if (pub.qos == Qos::AtLeastOnce) {
        if (client->awaiting_puback.count(pub.packet_id)) {
            connAck(client, PacketType::Puback, pub.packet_id);
            return;
        }
        client->awaiting_puback.emplace(pub.packet_id, pub.topic);
    }
    else if (pub.qos == Qos::ExactlyOnce) {
        if (client->awaiting_pubrel.count(pub.packet_id)) {
            connAck(client, PacketType::Pubrec, pub.packet_id);
            return;
        }
        client->awaiting_pubrel.emplace(pub.packet_id, pub.topic);
    }

    // Plugins can discard or modify messages
    if (!service.hooks.onMessageIn(pub, client->client_id)) {
        return;
    }

    // Empty retained messages shall be cleared instead of being forwarded.
    if (pub.retain && pub.payload.empty()) {
        // TODO: clear retained messages, do not forward
        if (pub.qos == Qos::AtLeastOnce)
            connAck(client, PacketType::Puback, pub.packet_id);
        else if (pub.qos == Qos::ExactlyOnce)
            connAck(client, PacketType::Pubrec, pub.packet_id);
        return;
    }

    Message msg{pub.topic, pub.payload, pub.qos, pub.retain, client->client_id};
    if (pub.retain) {
        // TODO: store retain message
    }
    Router router(service);
    router.route(msg);
    
    if (pub.qos == Qos::AtLeastOnce)
        connAck(client, PacketType::Puback, pub.packet_id);
    else if (pub.qos == Qos::ExactlyOnce)
        connAck(client, PacketType::Pubrec, pub.packet_id);
}

