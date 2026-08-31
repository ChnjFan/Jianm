/*
 * File: /TopicTree.cpp
 * Project: broker
 * Created Date: 2026-08-24 22:18:11
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 23:05:48
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

#include "TopicTree.hpp"

#include <algorithm>

#include "jianm/model/Topic.hpp"

using namespace jianm::broker;

void TopicTree::add(std::shared_ptr<Session> session, const std::string &filter, Qos qos)
{
    Node* node = root_.get();
    for (const auto& token : splitTopic(filter)) {
        auto it = node->children.find(token);
        if (it == node->children.end()) {
            it = node->children.emplace(token, std::make_unique<Node>()).first;
        }
        node = it->second.get();
    }

    // Duplicate subscription with the same (session, filter), update QoS
    for (auto& m : node->subscribers) {
        auto sub_session = m.session.lock();
        if (sub_session == session && m.filter == filter) {
            m.qos = qos;
            return;
        }
    }
    node->subscribers.push_back({session, filter, qos});
}

void TopicTree::remove(std::shared_ptr<Session> session, const std::string &filter)
{
    Node* node = root_.get();
    for (const auto& token : splitTopic(filter)) {
        auto it = node->children.find(token);
        if (it == node->children.end()) return;
        node = it->second.get();
    }

    auto& subs = node->subscribers;
    subs.erase(std::remove_if(subs.begin(), subs.end(),
        [&](const Match& m) {
            auto sub_session = m.session.lock();
            return sub_session == session && m.filter == filter;
        }), subs.end());
}

std::vector<TopicTree::Match> TopicTree::match(const std::string &topic) const
{
    std::vector<TopicTree::Match> out;
    collect(root_.get(), splitTopic(topic), 0, out);
    return out;
}

void TopicTree::collect(const Node *node, const std::vector<std::string> &tokens, size_t idx,
     std::vector<Match> &out) const
{
    if (!node) return;
    if (idx == tokens.size()) {
        out.insert(out.end(), node->subscribers.begin(), node->subscribers.end());
        // "a/#" also matches the parent "a"
        if (auto it = node->children.find("#"); it != node->children.end()) {
            out.insert(out.end(), it->second->subscribers.begin(), it->second->subscribers.end());
        }
        return;
    }
    // "#" matches all remaining levels subscribers
    if (auto it = node->children.find("#"); it != node->children.end()) {
        out.insert(out.end(), it->second->subscribers.begin(), it->second->subscribers.end());
    }
    // "+" matches any current layer subscribers
    if (auto it = node->children.find("+"); it != node->children.end()) {
        collect(it->second.get(), tokens, idx + 1, out);
    }
    // Precision subscribers
    if (auto it = node->children.find(tokens[idx]); it != node->children.end()) {
        collect(it->second.get(), tokens, idx + 1, out);
    }
}
