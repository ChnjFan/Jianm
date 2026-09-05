/*
 * File: /TickServiice.hpp
 * Project: broker
 * Created Date: 2026-09-05 11:34:59
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 11:59:04
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

#include <functional>
#include <vector>
#include <asio.hpp>

#include "common/Logger.hpp"
#include "Services.hpp"

namespace jianm {
namespace broker {


using clock = std::chrono::steady_clock;
using time_point = std::chrono::time_point<clock>;

class TickService
{
public:
    using TickCallback = std::function<void(BrokerServices& svc, const time_point& now)>;

    TickService(asio::io_context& io_context, int interval_ms, BrokerServices& services)
        : io_context_(io_context)
        , interval_ms_(interval_ms)
        , timer_(io_context_, std::chrono::milliseconds(interval_ms_))
        , services_(services) {}

    void start() { doTick(); }
    void stop() { timer_.cancel(); }

    void registerTask(TickCallback callback) { callback_.push_back(std::move(callback)); }
    
    void setInterval(int interval_ms) {
        interval_ms_ = interval_ms;
        // interval_ms_ is the tick interval in milliseconds, maybe adjustable in the future
        timer_.expires_after(std::chrono::milliseconds(interval_ms_));
    }

private:
    void doTick() {
        timer_.async_wait([this](const asio::error_code& ec) {
            if (ec) {
                JM_LOG_WARN("TickService timer error: {}", ec.message());
            }
            auto now = clock::now();
            for (const auto& callback : callback_) {
                try {
                    callback(services_, now);
                } catch (const std::exception& e) {
                    JM_LOG_ERROR("TickService callback error: {}", e.what());
                }
            }
            tick_count_++;
            doTick();
        });
    }

    asio::io_context& io_context_;
    int interval_ms_;
    asio::steady_timer timer_;
    BrokerServices& services_;
    size_t tick_count_ = 0;
    std::vector<TickCallback> callback_;
};

} // namespace broker
} // namespace jianm
