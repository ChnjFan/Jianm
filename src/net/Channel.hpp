/*
 * File: /Channel.hpp
 * Project: net
 * Created Date: 2026-08-22 17:08:11
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 12:55:17
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

namespace jianm {
namespace net {

using tcp = asio::ip::tcp;

typedef std::function<void(const asio::error_code&)> ReadFinishedCallback;

class Channel : public std::enable_shared_from_this<Channel> {
public:
    explicit Channel(asio::io_context &io_context);
    ~Channel();

    void start();
    void close();

    tcp::socket& getSocket() { return socket_; }

    bool isConnected() const { return connected_; }
    // set connected when bind session
    void setConnected() { connected_ = true; }
    void asyncSend(std::vector<uint8_t>&& buffer);

    // Peer endpoint accessors
    std::string getPeerIp() const { return peerEndpoint_.address().to_string(); }
    uint16_t getPeerPort() const { return peerEndpoint_.port(); }
    const tcp::endpoint& getPeerEndpoint() const { return peerEndpoint_; }

private:
    void asyncReadHead();
    void asyncRemainingLen(size_t offset);
    void asyncReadPayload(size_t offset, size_t size);

    void asyncReadSome(const size_t readSize, const size_t totalSize, const ReadFinishedCallback& callback);

    void asyncSend();

    bool connected_;
    bool closing_ = false;
    tcp::socket socket_;
    tcp::endpoint peerEndpoint_;

    std::vector<uint8_t> buffer_;

    std::mutex mtx_;
    std::queue<std::vector<uint8_t>> sendList_;
};

} // namespace net
} // namespace jianm
