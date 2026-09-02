# Jianm

English | [中文](./docs/README-zh.md)

Build your own MQTT broker from scratch.

Jianm is a minimal, functional MQTT broker inspired by [mosquitto](https://github.com/eclipse-mosquitto/mosquitto). It is built as a learning project to deeply understand the MQTT protocol. Currently it supports MQTT 3.1.1 packets and is based on the Asio networking library. The implementation process is documented in a blog series: "[Build MQTT Broker](https://chnjfan.github.io/tags/build-mqtt-broker/)".

## Project Structure

```
Jianm/
├── CMakeLists.txt          # Top-level CMake build configuration
├── conf/
│   └── jianm.conf          # Server configuration file
├── include/
│   └── jianm/              # Public headers
│       ├── api/            # Public API (BrokerEngine)
│       ├── contracts/      # Interface contracts (ITransport, IPacketHandler, IPlugin)
│       └── model/          # Data models (Packet, Session, Subscription)
├── src/
│   ├── main.cpp            # Entry point
│   ├── broker/             # Broker core (engine, handlers, session management)
│   ├── common/             # Common modules (config, logging, utilities)
│   ├── management/         # Management module (AdminServer, AdminSession)
│   ├── net/                # Network layer (Channel, ChannelFactory, TcpTransport)
│   ├── plugin/             # Plugin system (HookRegistry)
│   └── protocol/           # MQTT protocol codec (Codec)
├── plugins/
│   └── example/            # Example plugins
├── test/                   # Tests (GoogleTest-based)
├── thirdparty/
│   └── spdlog/             # Logging library (header-only mode)
└── build/                  # Build output directory
```

## Build

### Prerequisites

| Dependency    | Version | Description                              |
| ------------- | ------- | ---------------------------------------- |
| CMake         | ≥ 3.16  | Build system                             |
| Asio          | ≥ 1.18  | Cross-platform C++ networking library    |
| C++ Compiler  | C++17   | Compiler with C++17 support              |
| spdlog        | —       | Logging library (bundled, no install)    |
| GoogleTest    | v1.14.0 | Test framework (only needed for tests)   |

#### macOS

```bash
# Install CMake and Asio
brew install cmake asio

# C++ compiler: install Xcode Command Line Tools (auto-prompted on first use)
xcode-select --install
```

#### Linux (Ubuntu / Debian)

```bash
# Install build tools and CMake
sudo apt update
sudo apt install -y build-essential cmake

# Install Asio
sudo apt install -y libasio-dev
```

#### Linux (Fedora / CentOS / RHEL)

```bash
# Install build tools and CMake
sudo dnf install -y gcc gcc-c++ make cmake

# Install Asio
sudo dnf install -y asio-devel
```

> **spdlog** is bundled in the `thirdparty/` directory — no separate installation needed.
> **GoogleTest** is automatically downloaded by CMake when building tests.

### Quick Build

```bash
cd Jianm
cmake -S . -B build
cmake --build build
```

After building, the executable and configuration file are generated in `build/bin/`:

```
build/bin/
├── jianm          # MQTT Broker executable
└── jianm.conf     # Configuration file copy
```

### Build Options

| Option              | Default | Description                        |
| ------------------- | ------- | ---------------------------------- |
| `CMAKE_BUILD_TYPE`  | Release | Build type (Debug/Release/...)     |
| `JIANM_BUILD_TESTS` | ON      | Whether to build the test suite    |

Example: build debug version without tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DJIANM_BUILD_TESTS=OFF
cmake --build build
```

### Run Tests

```bash
cd build
ctest
```

Or run the test executable directly:

```bash
./build/bin/jianm_smoke_test
```

## Usage

### Start the Server

```bash
./build/bin/jianm
```

The server listens on two ports:

- **1883** — MQTT client connection port (standard MQTT port)
- **10000** — Admin management console port (telnet)

### Connect an MQTT Client

Use any MQTT client tool (e.g., `mosquitto_pub`/`mosquitto_sub`) to connect:

```bash
# Subscribe to a topic
mosquitto_sub -h localhost -p 1883 -t "test/topic"

# Publish a message
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "Hello Jianm"
```

### Management Console

Connect via telnet:

```bash
telnet localhost 10000
```

## Configuration

Server configuration is managed through the `conf/jianm.conf` file. It is automatically copied to the executable's directory during build.

```ini
# Log level (trace / debug / info / warn / error / critical)
log_level = trace

# MQTT server port (default 1883)
port = 1883

# Admin console port (default 10000)
admin_port = 10000

# Receive buffer size per connection (bytes, default 1024)
max_receive_size = 1024

# Allow anonymous connections (true = allowed)
allow_anonymous = true
```

If a configuration item is not set, the built-in default is used. A server restart is required for changes to take effect.

## Protocol Support

Based on MQTT 3.1.1 (OASIS Standard). Current implementation progress:

### Control Packets

| Packet Type | Direction       | Status | Description |
| ----------- | --------------- | :----: | ----------- |
| CONNECT     | Client → Server |   ✅   | Client connection request |
| CONNACK     | Server → Client |   ✅   | Connection acknowledgment |
| PUBLISH     | Client ↔ Server |   ✅   | Publish message |
| PUBACK      | Client ↔ Server |   ✅   | QoS 1 publish acknowledgment |
| PUBREC      | Client ↔ Server |   ✅   | QoS 2 publish received |
| PUBREL      | Client ↔ Server |   ⚠️   | QoS 2 publish release (codec supported, handler pending) |
| PUBCOMP     | Client ↔ Server |   ⚠️   | QoS 2 publish complete (codec supported, handler pending) |
| SUBSCRIBE   | Client → Server |   ❌   | Subscribe to topic |
| SUBACK      | Server → Client |   ❌   | Subscribe acknowledgment |
| UNSUBSCRIBE | Client → Server |   ❌   | Unsubscribe |
| UNSUBACK    | Server → Client |   ❌   | Unsubscribe acknowledgment |
| PINGREQ     | Client → Server |   ❌   | Ping request |
| PINGRESP    | Server → Client |   ❌   | Ping response |
| DISCONNECT  | Client → Server |   ❌   | Disconnect |

> ✅ Implemented　⚠️ Partially implemented　❌ Pending

### Connection Features

| Feature          | Status | Description |
| ---------------- | :----: | ----------- |
| Clean Session    |   ✅   | Clean session flag, controls session state clearing and restoration |
| Session Present  |   ✅   | Correctly calculated based on CleanSession and existing session state |
| Will Flag        |   ⚠️   | CONNECT packet parsing and validation supported; will message publishing not implemented |
| Username/Password | ⚠️  | CONNECT packet parsing, UTF-8 validation, allow_anonymous config supported; auth logic is a stub (always returns true) |
| Keep Alive       |   ✅   | 1.5× KeepAlive timeout auto-disconnect mechanism |
| ClientID Validation | ✅ | Length ≤ 23 chars, UTF-8 encoding validation |
| UTF-8 Validation |   ✅   | UTF-8 validation for ClientID, Username, Will Topic |
| Duplicate ClientID | ✅ | Same ClientID reconnect disconnects old connection (MQTT-3.1.4-2) |
| Duplicate CONNECT | ✅ | Second CONNECT on same connection treated as protocol violation (MQTT-3.1.0-2) |
| Admin Console    |   ✅   | Telnet port 10000, supports help/status/sessions/kick/quit commands |
| Logging          |   ✅   | Console and file output targets, configurable log level |
| Plugin System    |   ✅   | Observer pattern based on IPlugin interface, supports event hook extensions |

> ✅ Implemented　⚠️ Partially implemented　❌ Not implemented

### Messaging & Subscriptions

| Module | Feature | Status | Description |
| ------ | ------- | :----: | ----------- |
| Publish | PUBLISH packet parsing & routing | ✅ | TopicTree wildcard matching + Router delivery |
| Publish | Retained message storage | ⚠️ | Retained storage logic is TODO; parsing and clearing framework ready |
| Publish | QoS 1 message flow | ✅ | AT_LEAST_ONCE delivery + PUBACK + DUP retransmission dedup |
| Publish | QoS 2 message flow | ⚠️ | EXACTLY_ONCE receive + PUBREC done; PUBREL/PUBCOMP handling pending |
| Subscribe | SUBSCRIBE/SUBACK | ❌ | Topic subscription + granted QoS response |
| Subscribe | UNSUBSCRIBE/UNSUBACK | ❌ | Unsubscribe |
| Subscribe | Topic wildcard matching | ✅ | `+` single-level, `#` multi-level wildcards (TopicTree implementation) |
| Session | Session state persistence | ❌ | Save subscriptions and pending messages when CleanSession=0 |
| Session | Offline message queue | ❌ | Cache offline QoS 1/2 messages when CleanSession=0 |
| Heartbeat | PINGREQ/PINGRESP | ❌ | Client heartbeat request and server response |
| Disconnect | DISCONNECT handling | ❌ | Graceful client disconnection |
| Will | Will Message publishing | ❌ | Publish will message to subscribers on abnormal disconnect |
| Auth | Real authentication logic | ❌ | Currently a stub (always returns true) |

> ✅ Implemented　⚠️ Partially implemented　❌ Not implemented

## Plugin System

Jianm provides a plugin mechanism based on the `IPlugin` interface, allowing custom logic to be inserted at event points such as client connect, message receive, and disconnect.

### Plugin Interface

```cpp
class IPlugin {
    virtual std::string_view name() const = 0;
    virtual void onClientConnected(const std::string& client_id, const std::string& username) = 0;
    virtual bool onMessageIn(PublishPacket& msg, const std::string& client_id) = 0;
    virtual void onClientDisconnected(const std::string& client_id) = 0;
};
```

### Registering Plugins

```cpp
BrokerEngine broker(opts, ctx);
broker.addPlugin(std::make_unique<MyPlugin>());
broker.start();
```

### Built-in Plugins

| Plugin | Description |
| ------ | ----------- |
| MessagePrinterPlugin | Example plugin that prints client connect/disconnect events to stdout |
