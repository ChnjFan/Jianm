/*
 * File: /Message.cpp
 * Project: protocol
 * Created Date: 2026-08-21 17:19:17
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-23 12:56:07
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



#include "Message.hpp"

#include <unordered_map>

namespace jianm {
namespace protocol {

static const std::unordered_map<MessageType, const char*> messageTypeNameMap = {
    { MessageType::CONNECT,     "CONNECT" },
    { MessageType::CONNACK,     "CONNACK" },
    { MessageType::PUBLISH,     "PUBLISH" },
    { MessageType::PUBACK,      "PUBACK" },
    { MessageType::PUBREC,      "PUBREC" },
    { MessageType::PUBREL,      "PUBREL" },
    { MessageType::PUBCOMP,     "PUBCOMP" },
    { MessageType::SUBSCRIBE,   "SUBSCRIBE" },
    { MessageType::SUBACK,      "SUBACK" },
    { MessageType::UNSUBSCRIBE, "UNSUBSCRIBE" },
    { MessageType::UNSUBACK,   "UNSUBACK" },
    { MessageType::PINGREQ,     "PINGREQ" },
    { MessageType::PINGRESP,    "PINGRESP" },
    { MessageType::DISCONNECT,  "DISCONNECT" },
};

const char* messageTypeName(MessageType type) {
    auto it = messageTypeNameMap.find(type);
    if (it != messageTypeNameMap.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

} // namespace protocol
} // namespace jianm
