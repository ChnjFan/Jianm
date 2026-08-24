/*
 * File: /Channel.hpp
 * Project: net
 * Created Date: 2026-08-22 17:08:11
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 13:16:21
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

#include <memory>
#include <asio.hpp>

#include "jianm/contracts/ITransport.hpp"
#include "jianm/model/Packet.hpp"

namespace jianm {
namespace net {

using tcp = asio::ip::tcp;
using clock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<clock>;

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
using PacketPtr = std::shared_ptr<jianm::broker::Packet>;

class Channel : public std::enable_shared_from_this<Channel> {
public:
    explicit Channel(std::shared_ptr<jianm::broker::ITransport> transport);

    ~Channel();

    void start();
    void close();

    void requestClose(const std::string& reason);

    tcp::socket& getSocket() { return transport_->getSocket(); }

    bool asyncSend(const PacketPtr& packet);

    void setKeepalive(uint16_t seconds);
    std::string getPeer() const { return peer_; }

    // Event Callback, Injected by BrokerEngine
    std::function<void(const ChannelPtr&, const PacketPtr&)> on_packet;
    std::function<void(const ChannelPtr&, const std::string&)> on_close;

private:
    void asyncReadHead();
    void asyncRemainingLen(size_t offset);
    void asyncReadPayload(size_t offset, size_t size);


    std::shared_ptr<jianm::broker::ITransport> transport_;

    bool closing_ = false;
    [[maybe_unused]] bool close_posted_ = false;
    uint16_t keepalive_ = 0;
    time_point last_read_ = clock::now();
    std::string peer_;

    std::vector<uint8_t> buffer_;
};

} // namespace net
} // namespace jianm
