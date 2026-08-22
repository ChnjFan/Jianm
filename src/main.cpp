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

#include <iostream>
#include <memory>

#include "net/Server.hpp"
#include "net/AdminServer.hpp"
#include "common/ConfigMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"
#include "session/SessionMgr.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::string logLevel = jianm::common::ConfigMgr::getInstance()["log_level"];
    if (logLevel.empty())
        logLevel = jianm::common::DEFAULT_LOG_LEVEL;
    std::string logOutput = jianm::common::ConfigMgr::getInstance()["log_output"];
    if (logOutput.empty())
        logOutput = "console";
    jianm::common::logger_init(logLevel, logOutput);

    try
    {
        // Start SessionMgr thread first to handle request
        jianm::session::SessionMgr::getInstance()->start();

        asio::io_context io_context{1};
        unsigned short port = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["port"])
                                .value_or(jianm::common::DEFAULT_SERVER_PORT);
        std::make_shared<jianm::net::Server>(io_context, port)->start();
        JM_LOG_INFO("server started on port {}", port);

        // Start admin server on port 10000
        unsigned short adminPort = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["admin_port"])
                                     .value_or(jianm::common::DEFAULT_ADMIN_PORT);
        std::make_shared<jianm::net::AdminServer>(io_context, adminPort)->start();

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const asio::error_code &error, [[maybe_unused]] int signal_number) {
            if (error) {
                return;
            }
            io_context.stop();
        });

        io_context.run();

        JM_LOG_INFO("server close success");
    }
    catch(const std::exception& e)
    {
        JM_LOG_ERROR("server shutdown by: {}", e.what());
    }

    return 0;
}