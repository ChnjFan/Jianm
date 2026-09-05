/*
 * File: /BrokerEngine.cpp
 * Project: broker
 * Created Date: 2026-08-23 13:12:48
 * Author: ChnjFan
 * -----
 * Last Modified: 2026-09-05 13:30:07
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


#include <thread>
#include <asio.hpp>
#include <queue>

#include "jianm/api/BrokerEngine.hpp"

#include "common/Logger.hpp"
#include "common/ConfigMgr.hpp"
#include "net/Channel.hpp"
#include "net/ChannelFactory.hpp"
#include "management/AdminServer.hpp"
#include "protocol/Codec.hpp"
#include "plugin/HookRegistry.hpp"

#include "ClientContext.hpp"
#include "SessionManager.hpp"
#include "PacketDispatcher.hpp"
#include "Handlers.hpp"
#include "Services.hpp"
#include "TopicTree.hpp"
#include "Router.hpp"
#include "TickServiice.hpp"


using namespace jianm::broker;

using tcp = asio::ip::tcp;
using ClientPacketPair = std::pair<std::shared_ptr<ClientContext>, jianm::protocol::PacketPtr>;

class BrokerEngine::Impl : public std::enable_shared_from_this<Impl> {
public:
    Impl(BrokerEngine::Options opts, asio::io_context& ctx);
    ~Impl();
    
    bool start();
    void stop();
    bool running() const { return started_; };

    void addPlugin(std::unique_ptr<IPlugin> plugin) { hooks_.add(std::move(plugin)); }

private:
    static jianm::net::ChannelFactory::Config makeFactoryConfig(const BrokerEngine::Options& opts);

    void listen();
    
    void registerHandlers();

    void onPacket(std::shared_ptr<ClientContext> ctx, const jianm::protocol::PacketPtr& packet);
    void onClose(const jianm::net::ChannelPtr& channel, const std::string& reason);

    void handlePacket();
    void handleRequest();

    void registerTickTasks();

    Options opts_;
    asio::io_context& io_context_;
    tcp::acceptor acceptor_;

    jianm::net::ChannelFactory factory_;
    TopicTree topics_;
    SessionManager sessions_;
    jianm::plugin::HookRegistry hooks_;

    BrokerServices services_;
    TickService tick_service_;
    PacketDispatcher dispachter_;
    std::shared_ptr<jianm::management::AdminServer> admin_;

    bool started_ = false;

    std::mutex channel_mtx_;
    std::unordered_map<jianm::net::Channel*, jianm::net::ChannelPtr> channels_;

    std::mutex mtx_;
    std::condition_variable cond_;
    std::queue<ClientPacketPair> requests_;
    std::thread worker_;
};

BrokerEngine::Impl::Impl(BrokerEngine::Options opts, asio::io_context& ctx)
    : opts_(opts)
    , io_context_(ctx)
    , acceptor_(io_context_, tcp::endpoint(tcp::v4(), opts_.port))
    , factory_(makeFactoryConfig(opts), ctx)
    , topics_()
    , sessions_()
    , services_(topics_, sessions_, hooks_)
    , tick_service_(io_context_, opts_.tick_interval, services_)
    , admin_(std::make_shared<jianm::management::AdminServer>(opts.admin_port, services_))
{
    std::string logLevel = jianm::common::ConfigMgr::getInstance()["log_level"];
    if (logLevel.empty())
        logLevel = jianm::common::DEFAULT_LOG_LEVEL;
    std::string logOutput = jianm::common::ConfigMgr::getInstance()["log_output"];
    if (logOutput.empty())
        logOutput = "console";
    jianm::common::logger_init(logLevel, logOutput);

    registerHandlers();
    registerTickTasks();
}

BrokerEngine::Impl::~Impl()
{
    stop();
}

bool BrokerEngine::Impl::start()
{
    if (started_) return true;

    tick_service_.start();
    admin_->start();
    listen();

    started_ = true;
    worker_ = std::thread(&BrokerEngine::Impl::handlePacket, this);
    JM_LOG_INFO("server started on port {}", opts_.port);
    return true;
}

void BrokerEngine::Impl::stop()
{
    if (!started_) return;
    started_ = false;
    sessions_.stop();
    if (admin_) admin_->stop();

    // Wait for the message processing to complete
    if (worker_.joinable()) worker_.join();
}

jianm::net::ChannelFactory::Config BrokerEngine::Impl::makeFactoryConfig(const BrokerEngine::Options &opts)
{
    jianm::net::ChannelFactory::Config config;

    config.type = opts.transport_type;
    config.enable_websocket = opts.enable_websocket;
    config.ssl_cert_path = opts.ssl_cert_path;
    config.ssl_key_path = opts.ssl_key_path;

    return config;
}

void BrokerEngine::Impl::listen()
{
    auto self = shared_from_this();

    // Create channel wait for TCP
    auto channel = factory_.create();
    {
        std::lock_guard<std::mutex> lock(channel_mtx_);
        channels_.insert({channel.get(), channel});
    }

    [[maybe_unused]] auto ctx = sessions_.addChannel(channel);
    // Bind packet‑reception and disconnection callbacks
    channel->on_packet = [this, self](const jianm::net::ChannelPtr& channel,
                            const jianm::protocol::PacketPtr& packet) {
        auto ctx = sessions_.byChannel(channel);
        if (ctx) {
            onPacket(ctx, packet);
        }
    };
    channel->on_close = [this, self](const jianm::net::ChannelPtr& channel,
                            const std::string& reason) {
        onClose(channel, reason);
    };

    acceptor_.async_accept(channel->getSocket(), [self, channel](const asio::error_code &ec) {
        try {
            if (ec) {
                self->start();
                return;
            }
            channel->start();
            self->listen();
        } catch (const std::exception& e) {
            JM_LOG_ERROR("channel accept error: {}", e.what());
        }
    });
}

void BrokerEngine::Impl::registerHandlers()
{
    dispachter_.registerHandler(PacketType::Connect, std::make_unique<ConnectHandler>());
    dispachter_.registerHandler(PacketType::Publish, std::make_unique<PublishHandler>());
    dispachter_.registerHandler(PacketType::Subscribe, std::make_unique<SubscribeHandler>());
    dispachter_.registerHandler(PacketType::Unsubscribe, std::make_unique<UnsubscribeHandler>());
    dispachter_.registerHandler(PacketType::Puback, std::make_unique<AckHandler>());
    dispachter_.registerHandler(PacketType::Pubrec, std::make_unique<AckHandler>());
    dispachter_.registerHandler(PacketType::Pubrel, std::make_unique<AckHandler>());
    dispachter_.registerHandler(PacketType::Pubcomp, std::make_unique<AckHandler>());
    dispachter_.registerHandler(PacketType::Pingreq, std::make_unique<PingreqHandler>());
    dispachter_.registerHandler(PacketType::Disconnect, std::make_unique<DisconnectHandler>());
}

void BrokerEngine::Impl::onPacket(std::shared_ptr<ClientContext> ctx, const jianm::protocol::PacketPtr &packet)
{
    if (!ctx) {
        JM_LOG_WARN("received packet from unknown channel, dropping");
        return;
    }
    std::lock_guard<std::mutex> lock(mtx_);
    requests_.push({ctx, packet});
    cond_.notify_one();
}

void BrokerEngine::Impl::onClose(const jianm::net::ChannelPtr &channel, const std::string& reason)
{
    auto ctx = sessions_.byChannel(channel);
    if (ctx) {
        std::lock_guard<std::mutex> lock(channel_mtx_);
        channels_.erase(channel.get());
    }
    else {
        JM_LOG_INFO("onClose not found ctx");
        return;
    }
    
    const std::string& cid = ctx->client_id;
    // Abnormal disconnection with will requires publishing the will
    // no publishing for take‑over or normal disconnection
    if (ctx->connected && !ctx->clean_disconnect && !ctx->taken_over && ctx->will.valid) {
        Router router(services_);
        router.route({ctx->will.topic, ctx->will.payload, ctx->will.qos, ctx->will.retain, cid});
        JM_LOG_INFO("will published for {}", cid);
    }

    services_.hooks.onClientDisconnected(cid);
    if (!cid.empty()) {
        JM_LOG_INFO("client dissconnected: {} ({})", cid, reason);
    }

    sessions_.removeChannel(channel);
    std::lock_guard<std::mutex> lock(channel_mtx_);
    channels_.erase(channel.get());
}

void BrokerEngine::Impl::handlePacket()
{
    while (started_) {
        handleRequest();
    }
}

void BrokerEngine::Impl::handleRequest()
{
    jianm::protocol::PacketPtr packet;
    std::shared_ptr<ClientContext> ctx;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cond_.wait_for(lock, std::chrono::milliseconds(1000), [this](){
            if (!started_) return true;
            return !requests_.empty();
        });

        // The current message must be fully processed before termination
        // regardless of whether it needs to be closed.
        if (!requests_.empty()) {
            std::tie(ctx, packet) = requests_.front();
            requests_.pop();
        }
    }
    if (!packet) return;
    try {
        dispachter_.dispatch(services_, ctx, packet);
    }
    catch(const std::exception& e) {
        JM_LOG_WARN("protocol error from {} : {}", ctx->client_id, e.what());
        auto channel = ctx->channel.lock();
        if (channel) channel->requestClose(e.what());
    }
}

void BrokerEngine::Impl::registerTickTasks()
{
    tick_service_.registerTask([this](BrokerServices& svc, const time_point& now) {
        std::vector<jianm::net::ChannelPtr> snapshot;
        {
            std::lock_guard<std::mutex> lock(channel_mtx_);
            for (const auto& [_, channel] : channels_) {
                snapshot.push_back(channel);
            }
        }
        svc.sessions.checkKeepalive(now, snapshot);
    });

    tick_service_.registerTask([this](BrokerServices& svc, const time_point& now) {
        Router router(services_);
        svc.sessions.checkRetransmission(now, router);
    });
}

BrokerEngine::BrokerEngine(Options opts, asio::io_context& ctx)
    : impl_(std::make_unique<Impl>(std::move(opts), ctx)) {}

BrokerEngine::~BrokerEngine() = default;

bool BrokerEngine::start()
{
    return impl_->start();
}

void BrokerEngine::stop()
{
    return impl_->stop();
}

bool BrokerEngine::running() const
{
    return impl_->running();
}

void BrokerEngine::addPlugin(std::unique_ptr<IPlugin> plugin)
{
    return impl_->addPlugin(std::move(plugin));
}
