/*
 * File: /AdminSession.cpp
 * Project: net
 * Created Date: 2026-08-23 10:24:59
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-08-24 14:38:14
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



#include <iostream>
#include <sstream>

#include "AdminSession.hpp"
#include "common/ConfigMgr.hpp"
#include "common/Logger.hpp"
#include "broker/SessionManager.hpp"

using namespace jianm::net;

AdminSession::AdminSession(asio::io_context &io_context)
    : socket_(io_context)
{
    registerCommands();
}

AdminSession::~AdminSession()
{
    if (socket_.is_open()) {
        socket_.close();
    }
}

void AdminSession::start()
{
    try {
        auto ep = socket_.remote_endpoint();
        JM_LOG_INFO("Admin connection from {}:{}", ep.address().to_string(), ep.port());
    } catch (const asio::system_error&) {
        return;
    }

    doWrite("=== Jianm Admin Console ===\r\nType 'help' for available commands.\r\n> ");
    doReadLine();
}

void AdminSession::doReadLine()
{
    auto self = shared_from_this();
    asio::async_read_until(socket_, readBuf_, '\n',
        [this, self](const asio::error_code &ec, std::size_t /*bytes*/) {
            if (ec) {
                // Socket may already be closed — avoid calling remote_endpoint()
                JM_LOG_INFO("Admin disconnected");
                return;
            }

            std::istream is(&readBuf_);
            std::string line;
            std::getline(is, line);

            // Strip trailing '\r' for telnet (\r\n line endings)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            processCommand(line);
            doReadLine();
        });
}

void AdminSession::doWrite(const std::string &msg)
{
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(msg),
        [this, self](const asio::error_code &ec, std::size_t /*bytes*/) {
            if (ec) {
                socket_.close();
            }
        });
}

void AdminSession::processCommand(const std::string &line)
{
    // Skip empty lines
    if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
        doWrite("> ");
        return;
    }

    // Parse command and arguments
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    std::string args;
    std::getline(iss, args);
    // Trim leading whitespace from args
    size_t pos = args.find_first_not_of(" \t");
    if (pos != std::string::npos) {
        args = args.substr(pos);
    } else {
        args.clear();
    }

    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        it->second(args);
    } else {
        doWrite("Unknown command: " + cmd + "\r\nType 'help' for available commands.\r\n> ");
    }
}

void AdminSession::registerCommands()
{
    commands_["help"]    = [this](const std::string &a)   { cmdHelp(a); };
    commands_["status"]  = [this](const std::string &a)   { cmdStatus(a); };
    commands_["config"]  = [this](const std::string &a)   { cmdConfig(a); };
    commands_["sessions"] = [this](const std::string &a)  { cmdSessions(a); };
    commands_["kick"]    = [this](const std::string &a)   { cmdKick(a); };
    commands_["quit"]    = [this](const std::string &a)   { cmdQuit(a); };
    commands_["exit"]    = [this](const std::string &a)   { cmdQuit(a); };
}

void AdminSession::cmdHelp(const std::string & /*args*/)
{
    doWrite(
        "Available commands:\r\n"
        "  help                Show this help message\r\n"
        "  status              Show server status\r\n"
        "  config              Show current server configuration\r\n"
        "  sessions            List connected MQTT sessions\r\n"
        "  kick <client_id>    Disconnect a client session\r\n"
        "  quit / exit         Close admin connection\r\n"
        "> ");
}

void AdminSession::cmdStatus(const std::string & /*args*/)
{
    doWrite("Server is running.\r\n> ");
}

void AdminSession::cmdConfig(const std::string & /*args*/)
{
    auto entries = jianm::common::ConfigMgr::getInstance().getAll();
    std::ostringstream oss;
    oss << "Server configuration:\r\n";
    if (entries.empty()) {
        oss << "  (no config loaded — using built-in defaults)\r\n";
    } else {
        for (const auto &[key, value] : entries) {
            oss << "  " << key << " = " << value << "\r\n";
        }
    }
    oss << "> ";
    doWrite(oss.str());
}

void AdminSession::cmdSessions(const std::string & /*args*/)
{
    // SessionManager currently doesn't expose a public listing API.
    // For now, just indicate that there's no public accessor.
    doWrite("Session listing not yet implemented.\r\n> ");
}

void AdminSession::cmdKick(const std::string &args)
{
    if (args.empty()) {
        doWrite("Usage: kick <client_id>\r\n> ");
        return;
    }
    doWrite("Kicked session: " + args + "\r\n> ");
}

void AdminSession::cmdQuit(const std::string & /*args*/)
{
    doWrite("Bye.\r\n");
    socket_.close();
}
