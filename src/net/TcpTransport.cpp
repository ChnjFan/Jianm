/*
 * File: /TcpTransport.cpp
 * Project: net
 * Created Date: 2026-08-23 18:05:00
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-28 11:17:26
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

#include "TcpTransport.hpp"

#include <mutex>

using namespace jianm::net;

static const int MAX_SEND_QUEUE = 1024;

TcpTransport::TcpTransport(asio::io_context &ctx)
    : socket_(ctx)
{
}

void TcpTransport::asyncReadSome(std::vector<uint8_t> &buffer, const size_t readSize, const size_t totalSize,
        const ReadFinishedCallback &callback)
{
    auto self = shared_from_this();

    // Ensure the vector's logical size covers the region we want to read into.
    // totalSize is the absolute end position, not a relative length.
    if (buffer.size() < totalSize) {
        buffer.resize(totalSize);
    }

    socket_.async_read_some(asio::buffer(buffer.data() + readSize, totalSize - readSize),
        [this, self, &buffer, readSize, totalSize, callback](asio::error_code ec, std::size_t bytes_transferred) {
            if (ec || readSize + bytes_transferred >= totalSize) {
                callback(ec);
                return;
            }

            asyncReadSome(buffer, readSize + bytes_transferred, totalSize, callback);
        });
}

void TcpTransport::asyncSend(std::vector<uint8_t> &&buffer)
{
    std::lock_guard<std::mutex> lock(mtx_);
    const size_t sendSize = send_lists_.size();
    if (sendSize > MAX_SEND_QUEUE) {   // suppression
        return;
    }

    send_lists_.push(std::move(buffer));
    if (sendSize > 0) {
        return;
    }
    asyncSend();
}

void TcpTransport::close()
{
    closing_ = true;
    // If nothing is pending, close immediately; otherwise asyncSend callback
    // will close the socket after the queue drains.
    if (send_lists_.empty()) {
        socket_.close();
    }
}

void TcpTransport::asyncSend()
{
    const auto& buffer = send_lists_.front();
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(buffer),
        [self, this](const asio::error_code& error, [[maybe_unused]] size_t bytes_transfer) {
            if (error) {
                close();
                return;
            }

            std::lock_guard<std::mutex> lock(mtx_);
            send_lists_.pop();
            if (!send_lists_.empty()) {
                asyncSend();
            } else if (closing_) {
                socket_.close();
            }
        });
}
