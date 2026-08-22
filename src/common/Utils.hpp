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

#ifndef DEFER_HPP
#define DEFER_HPP

#include <functional>
#include <optional>
#include <string>

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

} // namespace common
} // namespace jianm

#endif
