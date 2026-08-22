/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Andrea Giacomo Baldan All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "Channel.hpp"
#include "common/ConfigMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"
#include "protocol/Packet.hpp"
#include "protocol/MessageMgr.hpp"
#include "protocol/mqtt.h"

using namespace jianm::net;

static const int DEFAULT_BUFFER_SIZE = 1024;
static const int MAX_SEND_QUEUE = 1024;

Channel::Channel(asio::io_context &io_context)
    : socket_(io_context)
{
    int configSize = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["max_receive_size"])
                         .value_or(DEFAULT_BUFFER_SIZE);
    buffer_.resize(configSize);
}

Channel::~Channel()
{
    close();
    JM_LOG_TRACE("TCP channel close success, peer {}:{}", getPeerIp(), getPeerPort());
}

void Channel::start()
{
    peerEndpoint_ = socket_.remote_endpoint();
    JM_LOG_TRACE("TCP channel accept success, peer {}:{}, start read head",
                 getPeerIp(), getPeerPort());

    asyncReadHead();
}

void Channel::close()
{
    socket_.close();
}

void Channel::asyncSend(std::vector<uint8_t>&& buffer)
{
    std::lock_guard<std::mutex> lock(mtx_);
    const size_t sendSize = sendList_.size();
    if (sendSize > MAX_SEND_QUEUE) {   // suppression
        return;
    }

    sendList_.push(std::move(buffer));
    if (sendSize > 0) {
        return;
    }
    asyncSend();
}

void Channel::asyncReadHead()
{
    auto self = shared_from_this();
    // Read the fixed header byte and first remaining length byte (offset 0..2)
    asyncReadSome(0, 2, [this, self](const asio::error_code& ec) {
        if (ec) {
            JM_LOG_TRACE("Channel {}:{} closed: {}", getPeerIp(), getPeerPort(), ec.message());
            return;
        }
        asyncRemainingLen(2);
    });
}

void Channel::asyncRemainingLen(size_t offset)
{
    auto self = shared_from_this();

    // Check the last byte read for the continuation bit (0x80)
    if (buffer_[offset - 1] & 0x80) {
        // there are more bytes to read for the remaining length
        asyncReadSome(offset, offset + 1, [this, self, offset](const asio::error_code& ec) {
            if (ec) {
                JM_LOG_ERROR("Channel {}:{} is reading remaining len closed: {}", getPeerIp(), getPeerPort(), ec.message());
                close();
            }
            asyncRemainingLen(offset + 1);
        });
        return;
    }

    int remainingLength = protocol::Packet::decodeRemainingLength(buffer_);
    const protocol::Header header = { .byte = buffer_[0] };
    JM_LOG_TRACE("Received packet header: type={} qos={} dup={} retain={} remaininglen={}",
        static_cast<int>(header.bits.type),
        static_cast<int>(header.bits.qos),
        static_cast<int>(header.bits.dup),
        static_cast<int>(header.bits.retain),
        remainingLength);

    // Read payload starting AFTER the header + remaining length bytes
    asyncReadPayload(offset, remainingLength);
}

void Channel::asyncReadPayload(size_t offset, size_t size)
{
    auto self = shared_from_this();
    // Read payload at the given offset, preserving bytes already in buffer
    asyncReadSome(offset, size + offset, [this, self](const asio::error_code& ec) {
        if (ec) {
            JM_LOG_ERROR("Channel {}:{} is reading payload closed: {}", getPeerIp(), getPeerPort(), ec.message());
            close();
            return;
        }
        // delivery data to protocol
        protocol::MessageMgr::getInstance()->messageHandle(self, buffer_);
        buffer_.clear();

        asyncReadHead();
    });
}


void Channel::asyncReadSome(const size_t readSize, const size_t totalSize, const ReadFinishedCallback &callback)
{
    auto self = shared_from_this();

    // Ensure the vector's logical size covers the region we want to read into.
    // Use the larger of current size or (readSize + totalSize) to preserve
    // already-read bytes (e.g. header + remaining length before payload).
    if (buffer_.size() < readSize + totalSize) {
        buffer_.resize(readSize + totalSize);
    }

    socket_.async_read_some(asio::buffer(buffer_.data() + readSize, totalSize - readSize),
        [this, self, readSize, totalSize, callback](asio::error_code ec, std::size_t bytes_transferred) {
            if (ec || readSize + bytes_transferred >= totalSize) {
                callback(ec);
                return;
            }

            asyncReadSome(readSize + bytes_transferred, totalSize, callback);
        });
}

void Channel::asyncSend()
{
    const auto& buffer = sendList_.front();
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(buffer),
        [self, this](const asio::error_code& error, [[maybe_unused]] size_t bytes_transfer) {
            if (error) {
                close();
                return;
            }

            std::lock_guard<std::mutex> lock(mtx_);
            sendList_.pop();
            if (!sendList_.empty()) {
                asyncSend();
            }
        });
}
