/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Andrea Giacomo Baldan All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ADMIN_SESSION_HPP
#define ADMIN_SESSION_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <asio.hpp>

namespace jianm {
namespace net {

using tcp = asio::ip::tcp;

class AdminSession : public std::enable_shared_from_this<AdminSession> {
public:
    AdminSession(asio::io_context &io_context);
    ~AdminSession();

    void start();
    tcp::socket& getSocket() { return socket_; }

private:
    void doReadLine();
    void doWrite(const std::string &msg);

    void processCommand(const std::string &line);
    void registerCommands();

    // Built-in commands
    void cmdHelp(const std::string &args);
    void cmdStatus(const std::string &args);
    void cmdConfig(const std::string &args);
    void cmdSessions(const std::string &args);
    void cmdKick(const std::string &args);
    void cmdQuit(const std::string &args);

    using CommandHandler = std::function<void(const std::string&)>;

    tcp::socket socket_;
    asio::streambuf readBuf_;
    std::unordered_map<std::string, CommandHandler> commands_;
};

} // namespace net
} // namespace jianm

#endif // ADMIN_SESSION_HPP
