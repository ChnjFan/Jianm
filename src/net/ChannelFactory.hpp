/*
 * File: /ChannelFactory.hpp
 * Project: net
 * Created Date: 2026-08-23 16:36:06
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 17:34:08
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

#include "jianm/model/Transport.hpp"
#include "Channel.hpp"

namespace jianm {
namespace net {

/**
 * @brief Connect Factory
 * 
 * Create specific Transport + Codec combinations according to listener configurations
 */
class ChannelFactory
{
public:
    struct Config {
        jianm::broker::TransportType type;

        std::string ssl_cert_path;
        std::string ssl_key_path;

        bool enable_websocket = false;
    };

    ChannelFactory(Config cfg, asio::io_context& ctx);
    ~ChannelFactory();

    std::shared_ptr<Channel> create() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace net
} // namespace jianm
