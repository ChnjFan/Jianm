/*
 * File: /IPlugin.hpp
 * Project: contracts
 * Created Date: 2026-08-24 17:54:06
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 19:05:31
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

#include "jianm/model/Packet.hpp"

#include <string>
#include <string_view>

namespace jianm {


/**
 * @brief Plugin Abstraction
 * 
 * IPlugin implementations can be dynamically loaded
 * by PluginManager via dlopen in the production environment
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Plugin name
     * 
     * @return std::string_view 
     */
    virtual std::string_view name() const = 0;

    /**
     * @brief Called when a client successfully connects
     *
     * @param client_id  The client identifier from the CONNECT packet
     * @param username   The username from the CONNECT packet (empty if absent)
     */
    virtual void onClientConnected(const std::string& client_id, const std::string& username) = 0;

    /**
     * @brief Called when a PUBLISH message is received from a client
     *
     * Implementations may inspect or modify the message before it is routed.
     * Return false to drop the message and prevent further processing.
     *
     * @param msg        The inbound PUBLISH packet (may be mutated in place)
     * @param client_id  The identifier of the publishing client
     * @return true      To continue normal message routing
     * @return false     To silently discard the message
     */
    virtual bool onMessageIn(PublishPacket& msg, const std::string& client_id) = 0;

    /**
     * @brief Called when a client disconnects (gracefully or abnormally)
     *
     * @param client_id  The identifier of the disconnected client
     */
    virtual void onClientDisconnected(const std::string& client_id) = 0;
};


} // namespace jianm

