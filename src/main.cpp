/*
 * File: /main.cpp
 * Project: src
 * Created Date: 2026-08-23 10:24:50
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 19:55:05
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



#include <iostream>
#include <memory>

#include "jianm/api/BrokerEngine.hpp"

#include "common/ConfigMgr.hpp"
#include "common/Utils.hpp"
#include "common/Logger.hpp"

#include "example/MessagePrinterPlugin.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try
    {
        jianm::broker::BrokerEngine::Options opts;
        opts.port = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["port"])
                                .value_or(jianm::common::DEFAULT_SERVER_PORT);
        opts.admin_port = jianm::common::parse_int(jianm::common::ConfigMgr::getInstance()["admin_port"])
                                     .value_or(jianm::common::DEFAULT_ADMIN_PORT);

        asio::io_context ctx{1};
        jianm::broker::BrokerEngine broker(opts, ctx);
        // broker.addPlugin(std::make_unique<jianm::example::MessagePrinterPlugin>());

        if (!broker.start()) {
            return 1;
        }

        asio::signal_set signals(ctx, SIGINT, SIGTERM);
        signals.async_wait([&ctx](const asio::error_code &error, [[maybe_unused]] int signal_number) {
            if (error) {
                return;
            }
            ctx.stop();
        });

        ctx.run();

        broker.stop();
        JM_LOG_INFO("server close success");
    }
    catch(const std::exception& e)
    {
        JM_LOG_ERROR("server shutdown by: {}", e.what());
    }

    return 0;
}