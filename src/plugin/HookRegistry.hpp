/*
 * File: /HookRegistry.hpp
 * Project: plugin
 * Created Date: 2026-08-24 19:09:09
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 19:11:25
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

#include <memory>
#include <string>
#include <vector>

namespace jianm {
namespace plugin {

/// @brief Plugin Registry (Observer Pattern): 
/// It broadcasts events at hook points, and plugins respond on demand.
class HookRegistry {
public:
    void add(std::unique_ptr<IPlugin> plugin) { plugins_.push_back(std::move(plugin)); }

    void onClientConnected(const std::string& client_id, const std::string& username) {
        for (auto& p : plugins_) p->onClientConnected(client_id, username);
    }

    // If any plugin returns false, the message will be discarded.
    bool onMessageIn(PublishPacket& msg, const std::string& client_id) {
        for (auto& p : plugins_) {
            if (!p->onMessageIn(msg, client_id)) return false;
        }
        return true;
    }

    void onClientDisconnected(const std::string& client_id) {
        for (auto& p : plugins_) p->onClientDisconnected(client_id);
    }

private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
};

}  // namespace plugin
}  // namespace jianm
