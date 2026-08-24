/*
 * File: /ChannelFactory.cpp
 * Project: net
 * Created Date: 2026-08-23 16:53:13
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 17:57:43
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

#ifdef HAS_OPENSSL
#include <asio/ssl.hpp>
#endif

#include "jianm/contracts/ITransport.hpp"
#include "common/Logger.hpp"
#include "ChannelFactory.hpp"
#include "TcpTransport.hpp"

using namespace jianm::net;

class ChannelFactory::Impl {
public:
    Impl(Config cfg, asio::io_context &ctx);
    
    std::shared_ptr<Channel> create();

private:
    Config config_;
    asio::io_context& io_context_;
#ifdef HAS_OPENSSL
    std::unique_ptr<asio::ssl::context> ssl_ctx_;
#endif
};

ChannelFactory::Impl::Impl(Config cfg, asio::io_context &ctx)
    : config_(std::move(cfg))
    , io_context_(ctx)
{
#ifdef HAS_OPENSSL
    ssl_ctx_ = std::make_unique(asio::ssl::context::tls_server);
    if (!config_.ssl_cert_path.empty() && !config_.ssl_key_path.empty()) {
        ssl_ctx_->use_certificate_chain_file(config_.ssl_cert_path);
        ssl_ctx_->use_private_key_file(config_.ssl_key_path);
    }
#else
    if (!config_.ssl_cert_path.empty()) {
        JM_LOG_WARN("TLS configured bt OPENSSL not available - TLS disabled");
    }
#endif
}

std::shared_ptr<Channel> ChannelFactory::Impl::create()
{
    std::shared_ptr<ITransport> transport;

    switch (config_.type)
    {
    case TransportType::ssl:
#ifdef HAS_OPENSSL
        if (!ssl_ctx_) {
            JM_LOG_WARN("TLS connection rejected: no SSL context configured");
            return nullptr;
        }

        // TODO: SslTransport
        return nullptr;
#else
        JM_LOG_WARN("TLS connection rejected: broker compiled without OpenSSL support");
        return nullptr;
#endif  
        break;
    case TransportType::tcp:
        [[fallthrough]];
    default:
        transport = std::make_shared<TcpTransport>(io_context_);
        break;
    }

    auto conn = std::make_shared<Channel>(transport);
    return conn;
}

ChannelFactory::ChannelFactory(Config cfg, asio::io_context &ctx)
    : impl_(std::make_unique<Impl>(std::move(cfg), ctx)) {}

ChannelFactory::~ChannelFactory() = default;

std::shared_ptr<Channel> ChannelFactory::create() const
{
    return impl_->create();
}
