/*
 * File: /Utils.hpp
 * Project: common
 * Created Date: 2026-08-22 21:11:04
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 12:53:53
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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>

using namespace std::chrono_literals;

namespace jianm {
namespace common {

class Defer {
public:
    explicit Defer(std::function<void()> func) : func_(std::move(func)) {};
    ~Defer() {
        func_();
    }

private:
    std::function<void()> func_;
};

inline void generate_client_id(std::string& value) {
    static const std::string cliendIDPrefix = "JianmClient_";
    static std::atomic<size_t> count = 0;
    if (count == SIZE_T_MAX) {
        count = 0;
    }
    value = cliendIDPrefix + std::to_string(count++);
}

/// Check if the elapsed time between two time points exceeds the given interval.
/// Uses steady_clock. Returns true if (now - last) >= interval.
inline bool is_timeout(const std::chrono::steady_clock::time_point &last,
                       const std::chrono::steady_clock::time_point &now,
                       const std::chrono::milliseconds &interval) {
    return (now - last) >= interval;
}

/// Check if the elapsed time since a past time point exceeds the given interval.
/// Uses steady_clock::now() as the current time.
/// Returns true if (steady_clock::now() - last) >= interval.
inline bool is_timeout(const std::chrono::steady_clock::time_point &last,
                       const std::chrono::milliseconds &interval) {
    return (std::chrono::steady_clock::now() - last) >= interval;
}

/// Safely parse a string to an int. Returns std::nullopt if the string is
/// empty, contains non-digit characters, or the value overflows.
inline std::optional<int> parse_int(const std::string& s) {
    try {
        size_t pos = 0;
        int value = std::stoi(s, &pos);
        // Reject trailing garbage like "123abc"
        if (pos != s.size()) {
            return std::nullopt;
        }
        return value;
    } catch (const std::invalid_argument&) {
        return std::nullopt;
    } catch (const std::out_of_range&) {
        return std::nullopt;
    }
}

/// Validate that a byte sequence is well-formed UTF-8.
/// Returns true if every code point follows the UTF-8 encoding rules:
///   1 byte:  0xxxxxxx
///   2 bytes: 110xxxxx 10xxxxxx
///   3 bytes: 1110xxxx 10xxxxxx 10xxxxxx
///   4 bytes: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
/// Rejects overlong encodings, surrogate halves (U+D800–U+DFFF), and
/// code points above U+10FFFF.
inline bool is_valid_utf8(const std::string& s) {
    const auto* bytes = reinterpret_cast<const uint8_t*>(s.data());
    size_t i = 0, n = s.size();

    while (i < n) {
        uint8_t lead = bytes[i];
        size_t seq_len;
        uint32_t code_point;

        if (lead < 0x80) {
            // ASCII, single byte
            ++i;
            continue;
        } else if ((lead >> 5) == 0x06) {
            // 2-byte sequence: 110xxxxx
            seq_len = 2;
            code_point = lead & 0x1F;
        } else if ((lead >> 4) == 0x0E) {
            // 3-byte sequence: 1110xxxx
            seq_len = 3;
            code_point = lead & 0x0F;
        } else if ((lead >> 3) == 0x1E) {
            // 4-byte sequence: 11110xxx
            seq_len = 4;
            code_point = lead & 0x07;
        } else {
            // Invalid lead byte (10xxxxxx or 11111xxx)
            return false;
        }

        // Not enough continuation bytes
        if (i + seq_len > n) {
            return false;
        }

        // Validate continuation bytes (10xxxxxx) and build the code point
        for (size_t j = 1; j < seq_len; ++j) {
            uint8_t cont = bytes[i + j];
            if ((cont >> 6) != 0x02) {
                return false;
            }
            code_point = (code_point << 6) | (cont & 0x3F);
        }

        // Reject overlong encodings
        if ((seq_len == 2 && code_point < 0x80)
            || (seq_len == 3 && code_point < 0x800)
            || (seq_len == 4 && code_point < 0x10000)) {
            return false;
        }

        // Reject surrogate halves and out-of-range code points
        if (code_point >= 0xD800 && code_point <= 0xDFFF) {
            return false;
        }
        if (code_point > 0x10FFFF) {
            return false;
        }

        i += seq_len;
    }

    return true;
}

} // namespace common
} // namespace jianm

