/*
 * File: /Topic.hpp
 * Project: model
 * Created Date: 2026-08-24 21:03:10
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 14:38:41
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

/**
 * @brief Check if a topic name is invalid according to MQTT rules.
 * 
 * @param topic 
 * @return true 
 * @return false 
 * 
 * MQTT 3.1.1 specification:
 * - PUBLISH Topic names must not contain wildcard characters ('+' or '#').
 */
inline bool isTopicNameInvalid(const std::string& topic) {
    if (topic.empty()) return true;
    for (auto c : topic) {
        // Wildcards are not allowed in published topics
        if (c == '+' || c == '#') return true;
    }
    return false;
}

/**
 * @brief Check if a topic filter is invalid according to MQTT rules.
 * 
 * @param filter 
 * @return true 
 * @return false 
 * 
 * MQTT 3.1.1 specification:
 * - SUBSCRIBE Topic filters can contain wildcards, but they must follow specific rules:
 * - The multi-level wildcard '#' must be the last character in the filter
 *       and must be preceded by a '/' if it's not the only character.
 * - The single-level wildcard '+' must occupy an entire level of the filter
 *       and must be either the only character in that level or surrounded by '/'.
 * 
 */
inline bool isTopicFilterInvalid(const std::string& filter) {
    if (filter.empty()) return true;
    const auto toks = splitTopic(filter);
    for (size_t i = 0; i < toks.size(); ++i) {
        const auto& t = toks[i];
        if (t == "#") {
            if (i != toks.size() - 1) return false;
        } else if (t == "+") {
            continue;  // Independent "+" legal
        } else if (t.find_first_of("+#") != std::string::npos) {
            return true;  // Wildcards mixed within a segment, e.g., "a#"/"b+c"
        }
    }
    return false;
}

/**
 * @brief Does the filter match the topic
 * 
 * @param filters 
 * @param topics 
 * @return true 
 * @return false 
 * 
 * '#' match all remaining levels, including parent levels
 */
inline bool matchTokens(const std::vector<std::string>& filters, const std::vector<std::string>& topics,
                        size_t fi = 0, size_t ti = 0) {
    if (fi == filters.size()) return ti == topics.size();
    if (filters[fi] == "#") return true;
    if (filters[fi] == "+") {
        if (ti >= topics.size()) return false;
        return matchTokens(filters, topics, fi + 1, ti + 1);
    }
    if (ti >= topics.size() || filters[fi] != topics[ti])
        return false;
    return matchTokens(filters, topics, fi + 1, ti + 1);
}

inline bool isTopicMatchesFilter(const std::string& filter, const std::string& topic) {
    return matchTokens(splitTopic(filter), splitTopic(topic));
}

} // namespace jianm
