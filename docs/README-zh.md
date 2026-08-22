# Jianm

Jianm 是一个最小可用的 MQTT Broker，模拟了 [mosquitto](https://github.com/eclipse-mosquitto/mosquitto) 的功能。

实现 MQTT Broker 主要是为了深入学习 MQTT 协议，目前这个项目支持 MQTT 3.1.1 协议报文，基于 Asio 网络库实现。实现过程记录在个人博客的系列文章「[C++ 实现 MQTT Broker：从协议到可运行代码]()」。

## 项目结构

```
Jianm/
├── CMakeLists.txt          # 顶层 CMake 构建配置
├── conf/
│   └── jianm.conf          # 服务器配置文件
├── src/
│   ├── main.cpp            # 程序入口
│   ├── common/             # 公共模块（配置、日志、工具）
│   ├── net/                # 网络层（Server、Channel、AdminServer）
│   ├── protocol/           # MQTT 协议解析（报文、消息）
│   └── session/            # 会话管理
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

| 特性           | 状态 | 说明 |
| -------------- | :--: | ---- |
| Clean Session  | ✅ | 清理会话标志位，控制会话状态清除 |
| Will Flag      | ⚠️ | CONNECT 报文解析支持，遗嘱消息发布未实现 |
| Username/Password | ⚠️ | CONNECT 报文解析支持，认证逻辑为桩函数（始终返回 true） |
| Keep Alive     | ⚠️ | CONNECT 报文解析支持，超时断开机制未实现 |

> ✅ 已实现　⚠️ 部分实现　❌ 未实现
