/*
 * File: /RetainStore.cpp
 * Project: broker
 * Created Date: 2026-09-05 14:10:23
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 14:13:16
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

#include "RetainStore.hpp"

using namespace jianm::broker;

void RetainStore::store(const std::string &topic, const Message &msg)
{
    map_[topic] = msg;
}

void RetainStore::clear(const std::string &topic)
{
    map_.erase(topic);
}

std::vector<jianm::Message> RetainStore::all() const
{
    std::vector<Message> out;
    out.reserve(map_.size());
    for (const auto& [topic, msg] : map_) {
        out.push_back(msg);
    }
    return out;
}
