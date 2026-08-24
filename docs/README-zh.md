# Jianm

Jianm 是一个最小可用的 MQTT Broker，模拟了 [mosquitto](https://github.com/eclipse-mosquitto/mosquitto) 的功能。

实现 MQTT Broker 主要是为了深入学习 MQTT 协议，目前这个项目支持 MQTT 3.1.1 协议报文，基于 Asio 网络库实现。实现过程记录在个人博客的系列文章「[C++ 实现 MQTT Broker：从协议到可运行代码](https://chnjfan.github.io/tags/build-mqtt-broker/)」。

## 项目结构

```
Jianm/
├── CMakeLists.txt          # 顶层 CMake 构建配置
├── conf/
│   └── jianm.conf          # 服务器配置文件
├── include/
│   └── jianm/              # 公共头文件
│       ├── api/            # 对外 API（BrokerEngine）
│       ├── contracts/      # 接口契约（ITransport、IPacketHandler、IPlugin）
│       └── model/          # 数据模型（Packet、Session、Subscription）
├── src/
│   ├── main.cpp            # 程序入口
│   ├── broker/             # Broker 核心（引擎、处理器、会话管理）
│   ├── common/             # 公共模块（配置、日志、工具）
│   ├── management/         # 管理模块（AdminServer、AdminSession）
│   ├── net/                # 网络层（Channel、ChannelFactory、TcpTransport）
│   ├── plugin/             # 插件系统（HookRegistry）
│   └── protocol/           # MQTT 协议编解码（Codec）
├── plugins/
│   └── example/            # 示例插件
├── test/                   # 测试（基于 GoogleTest）
├── thirdparty/
│   └── spdlog/             # 日志库（header-only 模式）
└── build/                  # 构建输出目录
```

## 构建

### 安装依赖

| 依赖       | 版本    | 说明                            |
| ---------- | ------- | ------------------------------- |
| CMake      | ≥ 3.16  | 构建系统                        |
| Asio       | ≥ 1.18  | 跨平台 C++ 网络库（standalone） |
| C++ 编译器 | C++17   | 支持 C++17 的编译器             |
| spdlog     | —       | 日志库（已内置，无需安装）      |
| GoogleTest | v1.14.0 | 测试框架（仅构建测试时需要）    |

#### macOS

```bash
# 安装 CMake 和 Asio
brew install cmake asio

# C++ 编译器：安装 Xcode Command Line Tools（首次使用时会自动提示安装）
xcode-select --install
```

#### Linux (Ubuntu / Debian)

```bash
# 安装编译工具和 CMake
sudo apt update
sudo apt install -y build-essential cmake

# 安装 Asio
sudo apt install -y libasio-dev
```

#### Linux (Fedora / CentOS / RHEL)

```bash
# 安装编译工具和 CMake
sudo dnf install -y gcc gcc-c++ make cmake

# 安装 Asio
sudo dnf install -y asio-devel
```

> **spdlog** 已包含在项目的 `thirdparty/` 目录中，无需单独安装。  
> **GoogleTest** 会在构建测试时由 CMake 自动下载，无需手动安装。

### 快速构建

```bash
cd Jianm
cmake -S . -B build
cmake --build build
```

构建完成后，可执行文件和配置文件会生成在 `build/bin/` 目录下：

```
build/bin/
├── jianm          # MQTT Broker 可执行文件
└── jianm.conf     # 配置文件副本
```

### 构建选项

| 选项                | 默认值  | 说明                          |
| ------------------- | ------- | ----------------------------- |
| `CMAKE_BUILD_TYPE`  | Release | 构建类型（Debug/Release/...） |
| `JIANM_BUILD_TESTS` | ON      | 是否构建测试套件              |

示例：构建调试版本并关闭测试

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DJIANM_BUILD_TESTS=OFF
cmake --build build
```

### 运行测试

```bash
cd build
ctest
```

或者直接运行测试可执行文件：

```bash
./build/bin/jianm_smoke_test
```

## 使用

### 启动服务端

```bash
./build/bin/jianm
```

服务端启动后会监听两个端口：

- **1883** — MQTT 客户端连接端口（标准 MQTT 端口）
- **10000** — Admin 管理控制台端口（telnet）

### 连接 MQTT 客户端

使用任意 MQTT 客户端工具（如 `mosquitto_pub`/`mosquitto_sub`）连接：

```bash
# 订阅主题
mosquitto_sub -h localhost -p 1883 -t "test/topic"

# 发布消息
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "Hello Jianm"
```

### 管理控制台

通过 telnet 连接管理控制台：

```bash
telnet localhost 10000
```

## 配置

服务端配置通过 `conf/jianm.conf` 文件管理。构建时该文件会自动复制到可执行文件同级目录。

```ini
# 日志级别（trace / debug / info / warn / error / critical）
log_level = trace

# MQTT 服务端端口（默认 1883）
port = 1883

# Admin 控制台端口（默认 10000）
admin_port = 10000

# 每个连接接收缓冲区大小（字节，默认 1024）
max_receive_size = 1024

# 是否允许匿名连接（true 表示允许）
allow_anonymous = true
```

如果配置项未设置，将使用内置默认值。修改配置后需重启服务端生效。

## 协议支持

基于 MQTT 3.1.1 协议（OASIS Standard）实现，当前开发进度如下：

### 控制报文

| 报文类型    | 方向        | 状态 | 说明 |
| ----------- | ----------- | :--: | ---- |
| CONNECT     | Client → Server | ✅ | 客户端连接请求 |
| CONNACK     | Server → Client | ✅ | 连接确认 |
| PUBLISH     | Client ↔ Server | ❌ | 发布消息 |
| PUBACK      | Client ↔ Server | ❌ | QoS 1 发布确认 |
| PUBREC      | Client ↔ Server | ❌ | QoS 2 发布收到 |
| PUBREL      | Client ↔ Server | ❌ | QoS 2 发布释放 |
| PUBCOMP     | Client ↔ Server | ❌ | QoS 2 发布完成 |
| SUBSCRIBE   | Client → Server | ❌ | 订阅主题 |
| SUBACK      | Server → Client | ❌ | 订阅确认 |
| UNSUBSCRIBE | Client → Server | ❌ | 取消订阅 |
| UNSUBACK    | Server → Client | ❌ | 取消订阅确认 |
| PINGREQ     | Client → Server | ❌ | 心跳请求 |
| PINGRESP    | Server → Client | ❌ | 心跳响应 |
| DISCONNECT  | Client → Server | ❌ | 断开连接 |

> ✅ 已实现　❌ 待实现

### 连接特性

| 特性             | 状态 | 说明 |
| ---------------- | :--: | ---- |
| Clean Session    | ✅ | 清理会话标志位，控制会话状态清除与恢复 |
| Session Present  | ✅ | 根据 CleanSession 和已有会话状态正确计算 |
| Will Flag        | ⚠️ | CONNECT 报文解析和校验支持，遗嘱消息发布未实现 |
| Username/Password | ⚠️ | CONNECT 报文解析、UTF-8 校验、allow_anonymous 配置支持，认证逻辑为桩函数（始终返回 true） |
| Keep Alive       | ✅ | 1.5× KeepAlive 超时自动断开机制 |
| ClientID 校验    | ✅ | 长度 ≤ 23 字符、UTF-8 编码校验 |
| UTF-8 校验       | ✅ | ClientID、Username、Will Topic 的 UTF-8 编码校验 |
| 重复 ClientID    | ✅ | 同一 ClientID 重连时断开旧连接（MQTT-3.1.4-2） |
| 重复 CONNECT     | ✅ | 同一连接重复发送 CONNECT 视为协议违规（MQTT-3.1.0-2） |
| Admin 管理控制台 | ✅ | Telnet 10000 端口，支持 help/status/sessions/kick/quit 命令 |
| 日志输出         | ✅ | 支持控制台和文件两种输出目标，可配置日志级别 |
| 插件系统         | ✅ | 基于 IPlugin 接口的观察者模式，支持事件钩子扩展 |

> ✅ 已实现　⚠️ 部分实现　❌ 未实现

### 消息与订阅（待实现）

| 模块 | 功能 | 状态 | 说明 |
| ---- | ---- | :--: | ---- |
| 发布 | PUBLISH 报文解析与路由 | ❌ | 消息发布到订阅者 |
| 发布 | Retained 消息存储 | ❌ | 保留新订阅者的最后一条消息 |
| 发布 | QoS 1 消息流 | ❌ | AT_LEAST_ONCE 投递 + PUBACK |
| 发布 | QoS 2 消息流 | ❌ | EXACTLY_ONCE 四步握手 |
| 订阅 | SUBSCRIBE/SUBACK | ❌ | 订阅主题 + granted QoS 返回 |
| 订阅 | UNSUBSCRIBE/UNSUBACK | ❌ | 取消订阅 |
| 订阅 | Topic 通配符匹配 | ❌ | `+` 单层、`#` 多层通配符 |
| 会话 | 会话状态持久化 | ❌ | CleanSession=0 时保存订阅和 pending 消息 |
| 会话 | 离线消息队列 | ❌ | CleanSession=0 时缓存离线 QoS 1/2 消息 |
| 心跳 | PINGREQ/PINGRESP | ❌ | 客户端心跳请求与服务端响应 |
| 断开 | DISCONNECT 处理 | ❌ | 客户端优雅断开连接 |
| 遗嘱 | Will Message 发布 | ❌ | 异常断开时向订阅者发布遗嘱消息 |
| 认证 | 真实认证逻辑 | ❌ | 当前为桩函数（始终返回 true） |

## 插件系统

Jianm 提供了基于 `IPlugin` 接口的插件机制，允许在客户端连接、消息收发、断开等事件点插入自定义逻辑。

### 插件接口

```cpp
class IPlugin {
    virtual std::string_view name() const = 0;
    virtual void onClientConnected(const std::string& client_id, const std::string& username) = 0;
    virtual bool onMessageIn(PublishPacket& msg, const std::string& client_id) = 0;
    virtual void onClientDisconnected(const std::string& client_id) = 0;
};
```

### 注册插件

```cpp
BrokerEngine broker(opts, ctx);
broker.addPlugin(std::make_unique<MyPlugin>());
broker.start();
```

### 内置插件

| 插件 | 说明 |
| ---- | ---- |
| MessagePrinterPlugin | 示例插件，将客户端连接/断开事件打印到 stdout |
