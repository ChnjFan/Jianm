/*
 * File: /Logger.hpp
 * Project: common
 * Created Date: 2026-08-22 17:25:58
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 13:31:23
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

#include <string>
#include <utility>

// Must be defined before including fmt headers so that all template
// definitions are instantiated in each translation unit (header-only mode).
#define FMT_HEADER_ONLY
#include <format.h>
#include <std.h>

namespace jianm {
namespace common {

const std::string DEFAULT_LOG_LEVEL = "debug";

/// Initialize the global logger. Safe to call multiple times — only the
/// first call takes effect. Must be called once before using JM_LOG_*.
/// @param level   Log level (trace/debug/info/warn/error/critical).
/// @param output  Output target: "console" for stdout, otherwise a file path.
void logger_init(const std::string& level = "info",
                const std::string& output = "console");

void log_trace(const std::string& msg);
void log_debug(const std::string& msg);
void log_info(const std::string& msg);
void log_warn(const std::string& msg);
void log_error(const std::string& msg);
void log_critical(const std::string& msg);

/// Format arguments using fmt (the same syntax spdlog uses). Returns a
/// std::string ready to be passed to the log_* functions.
template <typename... Args>
std::string format(fmt::format_string<Args...> fmt_str, Args&&... args) {
    return fmt::format(fmt_str, std::forward<Args>(args)...);
}

} // namespace common
} // namespace jianm

// ---------------------------------------------------------------------------
// Convenience macros — format with fmt, then forward to the log functions.
// Example: JM_LOG_INFO("server started on port {}", port);
// ---------------------------------------------------------------------------
#define JM_LOG_TRACE(...)    ::jianm::common::log_trace(::jianm::common::format(__VA_ARGS__))
#define JM_LOG_DEBUG(...)    ::jianm::common::log_debug(::jianm::common::format(__VA_ARGS__))
#define JM_LOG_INFO(...)     ::jianm::common::log_info(::jianm::common::format(__VA_ARGS__))
#define JM_LOG_WARN(...)     ::jianm::common::log_warn(::jianm::common::format(__VA_ARGS__))
#define JM_LOG_ERROR(...)    ::jianm::common::log_error(::jianm::common::format(__VA_ARGS__))
#define JM_LOG_CRITICAL(...) ::jianm::common::log_critical(::jianm::common::format(__VA_ARGS__))

