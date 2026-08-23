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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ConfigMgr.hpp"
#include "Logger.hpp"

using namespace jianm::common;


// Trim leading and trailing whitespace from a string
std::string trim(const std::string &str) {
    const auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    const auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

ConfigMgr::ConfigMgr() {
    const std::filesystem::path currentDir = std::filesystem::current_path();
    const auto configFilePath = currentDir / std::filesystem::path("jianm.conf");

    if (!std::filesystem::exists(configFilePath)) {
        JM_LOG_WARN("Not found config file: {}, will use default config", configFilePath);
        return;
    }

    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        JM_LOG_ERROR("failed to open config file: {}", configFilePath);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line.front() == '#' || line.front() == ';' || line.front() == '[') {
            continue;
        }

        const auto delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = trim(line.substr(0, delimiterPos));
            std::string value = trim(line.substr(delimiterPos + 1));
            configs_[key] = value;
        }
        else {
            std::string key = trim(line.substr(0, delimiterPos));
            configs_[key] = "true";
        }
    }
}

std::string ConfigMgr::operator[](const std::string &key) {
    if (configs_.find(key) == configs_.end()) {
        return {};
    }
    return configs_[key];
}

std::vector<std::pair<std::string, std::string>> ConfigMgr::getAll() const {
    std::vector<std::pair<std::string, std::string>> entries(configs_.begin(), configs_.end());
    std::sort(entries.begin(), entries.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    return entries;
}
