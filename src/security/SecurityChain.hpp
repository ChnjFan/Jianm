/*
 * File: /SecurityChain.hpp
 * Project: security
 * Created Date: 2026-09-05 22:59:25
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-06 10:01:56
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
#include <vector>

#include "jianm/contracts/IAuth.h"

namespace jianm {
namespace security {

/// @brief Security Responsibility Chain:
/// Authenticators are attempted in sequence, and access is granted if any one succeeds;
/// authorization is delegated to the registered Authorizer.
class SecurityChain
{
public:
    void addAuthenticator(std::unique_ptr<IAuthenticator> auth);
    void addAuthorizer(std::unique_ptr<IAuthorizer> authz);

    bool authenticate(const std::string& client_id, const std::string& username,
                      const std::string& password);
    bool canPublish(const std::string& client_id, const std::string& topic);
    bool canSubscribe(const std::string& client_id, const std::string& filter);

private:
    std::vector<std::unique_ptr<IAuthenticator>> auths_;
    std::unique_ptr<IAuthorizer> authz_;
};

/// @brief Default policy: Allow all after configuring allow_anonymous
class AllowAllAuthenticator : public IAuthenticator {
public:
    bool authenticate([[maybe_unused]]const std::string&, [[maybe_unused]]const std::string&,
        [[maybe_unused]]const std::string&) override {
        return true;
    }
};

class AllowAllAuthorizer : public IAuthorizer {
public:
    bool canPublish([[maybe_unused]]const std::string&, [[maybe_unused]]const std::string&) override {
        return true;
    }
    bool canSubscribe([[maybe_unused]]const std::string&, [[maybe_unused]]const std::string&) override {
        return true;
    }
};

/// @brief 未配置 allow_anonymous 默认采用用户密码认证
class PasswordAuthenticator : public IAuthenticator {
public:
    PasswordAuthenticator();
    bool authenticate(const std::string& client_id, const std::string& username,
                      const std::string& password) override;

private:
    std::vector<std::pair<std::string, std::string>> users_;
};

} // namespace security
} // namespace jianm

