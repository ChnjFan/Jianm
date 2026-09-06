/*
 * File: /SecurityChain.cpp
 * Project: security
 * Created Date: 2026-09-06 09:51:57
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-06 10:05:07
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

#include "SecurityChain.hpp"

using namespace jianm::security;

void SecurityChain::addAuthenticator(std::unique_ptr<IAuthenticator> auth) {
    auths_.push_back(std::move(auth));
}

void SecurityChain::addAuthorizer(std::unique_ptr<IAuthorizer> authz) {
    authz_ = std::move(authz);
}

bool SecurityChain::authenticate(const std::string &client_id, const std::string &username,
    const std::string &password) {
    for (const auto& auth : auths_) {
        if (auth->authenticate(client_id, username, password))
            return true;
    }
    return auths_.empty();
}

bool SecurityChain::canPublish(const std::string &client_id, const std::string &topic) {
    return authz_ ? authz_->canPublish(client_id, topic) : true;
}

bool SecurityChain::canSubscribe(const std::string &client_id, const std::string &filter) {
    return authz_ ? authz_->canSubscribe(client_id, filter) : true;
}

PasswordAuthenticator::PasswordAuthenticator()
{
    // TODO: Load user and password
}

bool PasswordAuthenticator::authenticate([[maybe_unused]]const std::string &client_id, const std::string &username,
     const std::string &password)
{
    // User unrestricted authentication is not configured
    if (users_.empty()) return true;
    for (const auto& [u, p] : users_) {
        if (u == username && p == password)
            return true;
    }
    return false;
}

