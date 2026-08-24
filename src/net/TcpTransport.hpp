/*
 * File: /TcpTransport.hpp
 * Project: net
 * Created Date: 2026-08-23 17:20:17
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 13:16:08
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

#include <asio.hpp>

#include "jianm/contracts/ITransport.hpp"

namespace jianm {
namespace net {

class TcpTransport 
    : public jianm::broker::ITransport
    , public std::enable_shared_from_this<TcpTransport>
{
public:
    explicit TcpTransport(asio::io_context &ctx);
    ~TcpTransport() override { close(); }

    void asyncReadSome(std::vector<uint8_t>& buffer, const size_t readSize, const size_t totalSize,
             const jianm::broker::ReadFinishedCallback& callback) override;
    void asyncSend(std::vector<uint8_t>&& buffer) override;
    void close() override;

    asio::ip::tcp::socket& getSocket() override { return socket_; };

private:
    void asyncSend();

    asio::ip::tcp::socket socket_;

    bool closing_ = false;

    std::mutex mtx_;
    std::queue<std::vector<uint8_t>> send_lists_;
};


} // namespace net
} // namespace jianm
