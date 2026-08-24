/*
 * File: /MessagePrinterPlugin.hpp
 * Project: example
 * Created Date: 2026-08-24 19:45:20
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 19:53:17
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

#include "jianm/contracts/IPlugin.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace jianm {
namespace example {

// Sample Plugin: Print each message; discard topics starting with "drop/" 
// demonstrates the intervention capability of plugins
class MessagePrinterPlugin : public IPlugin {
public:
    std::string_view name() const override { return "message-printer"; }

    void onClientConnected(const std::string& client_id, const std::string& username) override {
        std::cout << "[plugin] + client: " << client_id
                  << (username.empty() ? "" : " (user " + username + ")") << std::endl;
    }

    bool onMessageIn(PublishPacket& msg, const std::string& client_id) override {
        if (msg.topic.rfind("drop/", 0) == 0) {
            std::cout << "[plugin] drop: " << client_id << " -> " << msg.topic << std::endl;
            return false;
        }
        std::cout << "[plugin] msg: " << client_id << " -> " << msg.topic << " ("
                  << msg.payload.size() << "B)" << std::endl;
        return true;
    }

    void onClientDisconnected(const std::string& client_id) override {
        std::cout << "[plugin] - client: " << client_id << std::endl;
    }
};
    
} // namespace example
} // namespace jianm

