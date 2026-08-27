/*
 * File: /Channel.cpp
 * Project: net
 * Created Date: 2026-08-22 19:29:26
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 17:20:34
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

#include "Channel.hpp"

#include <stdexcept>

#include "common/ConfigMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"
#include "protocol/Codec.hpp"

using namespace jianm::net;

static const int DEFAULT_BUFFER_SIZE = 1024;

Channel::Channel(std::shared_ptr<jianm::ITransport> transport)
    : transport_(transport)
{
    int configSize = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["max_receive_size"])
                         .value_or(DEFAULT_BUFFER_SIZE);
    buffer_.resize(configSize);
}

Channel::~Channel()
{
    close();
    JM_LOG_TRACE("TCP channel close success, peer {}", peer_);
}

void Channel::start()
{
    auto endpoint = transport_->getSocket().remote_endpoint();
    peer_ = endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    JM_LOG_TRACE("TCP channel accept success, peer {}, start read head", peer_);

    asyncReadHead();
}

void Channel::close()
{
    transport_->close();
}

void Channel::requestClose(const std::string &reason)
{
    auto self = shared_from_this();

    if (on_close) {
        on_close(self, reason);
    }

    close();
}

bool Channel::asyncSend(const PacketPtr &packet)
{
    if (closing_) return false;
    try
    {
        std::vector<uint8_t> buffer;
        if (jianm::protocol::Codec::encode(packet, buffer)) {
            transport_->asyncSend(std::move(buffer));
        }
    }
    catch(const std::exception& e)
    {
        requestClose("write error");
    }
    return true;
}

void Channel::setKeepalive(uint16_t seconds)
{
    keepalive_ = seconds;
    last_read_ = clock::now();
}

void Channel::asyncReadHead()
{
    auto self = shared_from_this();
    // Read the fixed header byte and first remaining length byte (offset 0..2)
    transport_->asyncReadSome(buffer_, 0, 2,
        [this, self](const asio::error_code& ec) {
        if (ec) {
            JM_LOG_TRACE("Channel {} closed: {}", peer_, ec.message());
            requestClose("peer closed connection");
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
        transport_->asyncReadSome(buffer_, offset, offset + 1,
            [this, self, offset](const asio::error_code& ec) {
            if (ec) {
                JM_LOG_ERROR("Channel {} is reading remaining len closed: {}", peer_, ec.message());
                return;
            }
            asyncRemainingLen(offset + 1);
        });
        return;
    }

    size_t rlIndex = 1;  // remaining length starts after the first fixed header byte
    int remainingLength = static_cast<int>(protocol::Codec::decodeRemainingLength(buffer_, rlIndex));
    const jianm::Header header = { .byte = buffer_[0] };
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
    transport_->asyncReadSome(buffer_, offset, size + offset,
        [this, self](const asio::error_code& ec) {
        if (ec) {
            JM_LOG_ERROR("Channel {} is reading payload closed: {}", peer_, ec.message());
            close();
            return;
        }

        try {
            const jianm::protocol::PacketPtr pack = jianm::protocol::Codec::decode(buffer_);
            if (pack && on_packet) {
                on_packet(self, pack);
            }
            buffer_.clear();
            asyncReadHead();
        }
        catch(const std::exception& e) {
            requestClose(e.what());
        }
    });
}
