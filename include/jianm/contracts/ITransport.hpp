/*
 * File: /ITransport.hpp
 * Project: contracts
 * Created Date: 2026-08-23 16:40:31
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 13:49:15
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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <asio.hpp>

namespace jianm {
namespace broker {

typedef std::function<void(const asio::error_code& ec)> ReadFinishedCallback;

/**
 * @brief Transport‑Layer Abstraction
 * 
 * Respective Implementations of TCP / TLS / WebSocket
 */
class ITransport
{
public:
    virtual ~ITransport() = default;

    virtual void asyncReadSome(std::vector<uint8_t>& buffer, const size_t readSize, const size_t totalSize,
             const ReadFinishedCallback& callback) = 0;
    virtual void asyncSend(std::vector<uint8_t>&& buffer) = 0;

    virtual void close() = 0;

    virtual asio::ip::tcp::socket& getSocket() = 0;
};

} // namespace broker 
} // namespace jian 