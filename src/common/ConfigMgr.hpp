/*
 * File: /ConfigMgr.hpp
 * Project: common
 * Created Date: 2026-08-23 10:19:56
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 12:53:04
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

namespace jianm {
namespace common {

#include <unordered_map>
#include <string>
#include <vector>

constexpr unsigned short DEFAULT_SERVER_PORT = 1883;
constexpr unsigned short DEFAULT_ADMIN_PORT = 10000;
const std::string DEFAULT_LOG_LEVEL = "debug";

class ConfigMgr {
public:
    ~ConfigMgr() = default;

    static ConfigMgr& getInstance() {
        static ConfigMgr configMgr;
        return configMgr;
    }

    ConfigMgr(const ConfigMgr& other) = delete;
    ConfigMgr& operator=(const ConfigMgr& other) = delete;
    std::string operator[](const std::string& key);

    // Return all config entries as a sorted list of (key, value) pairs
    std::vector<std::pair<std::string, std::string>> getAll() const;

private:
    ConfigMgr();
    std::unordered_map<std::string, std::string> configs_;
};


} // namespace common
} // namespace jianm

