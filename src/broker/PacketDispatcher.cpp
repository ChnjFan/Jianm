/*
 * File: /PacketDispatcher.cpp
 * Project: broker
 * Created Date: 2026-08-23 15:51:08
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 12:04:57
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

#include "PacketDispatcher.hpp"

using namespace jianm::broker;


void PacketDispatcher::registerHandler(PacketType type, std::unique_ptr<IPacketHandler> handler)
{
    handlers_[static_cast<uint8_t>(type)] = std::move(handler);
}

void PacketDispatcher::dispatch(BrokerServices &svc, std::shared_ptr<ClientContext> &client,
     const std::shared_ptr<Packet> &pkt)
{
    auto it = handlers_.find(static_cast<uint8_t>(pkt->type));
    if (it == handlers_.end()) {
        throw std::runtime_error("unhandled packet type");
    }
    it->second->handle(svc, client, pkt);
}
