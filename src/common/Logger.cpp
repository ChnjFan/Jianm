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

#include "Logger.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>

namespace jianm {
namespace common {

void logger_init(const std::string& level) {
    // Guard against double initialization
    if (spdlog::get("jianm")) {
        return;
    }

    // Initialize a dedicated thread pool for async logging (8K queue, 1 worker)
    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(8192, 1);
    }

    auto console = spdlog::stdout_color_mt<spdlog::async_factory>("jianm");
    console->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");
    console->set_level(spdlog::level::from_str(level));
    spdlog::set_default_logger(console);
}

void log_trace(const std::string& msg) { spdlog::trace(msg); }
void log_debug(const std::string& msg) { spdlog::debug(msg); }
void log_info(const std::string& msg) { spdlog::info(msg); }
void log_warn(const std::string& msg) { spdlog::warn(msg); }
void log_error(const std::string& msg) { spdlog::error(msg); }
void log_critical(const std::string& msg) { spdlog::critical(msg); }

} // namespace common
} // namespace jianm
