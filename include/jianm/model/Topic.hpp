/*
 * File: /Topic.hpp
 * Project: model
 * Created Date: 2026-08-24 21:03:10
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-31 10:00:26
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
#include <vector>

namespace jianm {

inline std::vector<std::string> splitTopic(const std::string& topic) {
    std::vector<std::string> out;
    if (topic.empty()) return {""};
    size_t start = 0;
    while (true) {
        size_t pos = topic.find('/', start);
        if (pos == std::string::npos) {
            out.push_back(topic.substr(start));
            break;
        }
        out.push_back(topic.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}
    
inline bool isTopicNameInvalid(const std::string& topic) {
    if (topic.empty()) return false;
    for (auto c : topic) {
        // Wildcards are not allowed in published topics
        if (c == '+' || c == '#') return true;
    }
    return false;
}


} // namespace jianm
