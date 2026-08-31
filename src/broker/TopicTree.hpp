/*
 * File: /TopicTree.hpp
 * Project: broker
 * Created Date: 2026-08-24 22:12:41
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-31 09:48:11
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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jianm/model/Qos.hpp"
#include "jianm/model/Session.hpp"


namespace jianm {
namespace broker {
    
class TopicTree {
public:
    struct Match {
        std::weak_ptr<Session> session;
        std::string filter;
        Qos qos{Qos::AtMostOnce};
    };
    
    TopicTree() = default;
    ~TopicTree() = default;

    void add(std::shared_ptr<Session> session, const std::string& filter, Qos qos);
    void remove(std::shared_ptr<Session> session, const std::string& filter);
    std::vector<Match> match(const std::string& topic) const;

private:
    struct Node {
        std::unordered_map<std::string, std::unique_ptr<Node>> children;
        std::vector<Match> subscribers;
    };

    void collect(const Node* node, const std::vector<std::string>& tokens, size_t idx,
                std::vector<Match>& out) const;

    std::unique_ptr<Node> root_ = std::make_unique<Node>();
};


} // namespace broker
} // namespace jianm

