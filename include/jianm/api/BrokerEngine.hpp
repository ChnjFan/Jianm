/*
 * File: /BrokerEngine.hpp
 * Project: api
 * Created Date: 2026-08-23 11:52:29
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 19:14:15
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

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <asio.hpp>

#include "jianm/model/Transport.hpp"
#include "jianm/contracts/IPlugin.hpp"

namespace jianm {
namespace broker {

/**
 * @brief Main entry point for the Jianm broker engine.
 *
 * Provides a simple start/stop lifecycle for the broker. The engine binds to
 * a configurable host and port, accepts client connections, and routes MQTT
 * messages. Copying is explicitly disabled; use a single instance per process.
 */
class BrokerEngine {
public:
    struct Options {
        std::string host = "0.0.0.0";
        uint16_t port = 1883;
        uint16_t admin_port = 10000;
        std::string log_level = "debug";

        // Transport protocol
        TransportType transport_type = TransportType::tcp;

        // SSL/TLS Configuration (Optional)
        // Set the paths for certificate and private key to enable TLS
        std::string ssl_cert_path;
        std::string ssl_key_path;

        // WebSocket Configuration
        // Set to true to enable MQTT over WebSocket
        bool enable_websocket = false;
    };

    BrokerEngine(Options opts, asio::io_context& ctx);
    ~BrokerEngine();

    BrokerEngine(const BrokerEngine&) = delete;
    BrokerEngine& operator=(const BrokerEngine&) = delete;

    bool start();
    void stop();
    bool running() const;

    // Dependency Injection (Strategy/Plugin), must be called before start()
    void addPlugin(std::unique_ptr<IPlugin> plugin);

private:
    class Impl;
    std::shared_ptr<Impl> impl_;
};


} // namespace broker
} // namespace jianm
