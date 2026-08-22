# Jianm

English | [中文](./docs/README-zh.md)

Build your own MQTT broker from scratch.

Jianm is a minimal, functional MQTT broker inspired by [mosquitto](https://github.com/eclipse-mosquitto/mosquitto). It is built as a learning project to deeply understand the MQTT protocol. Currently it supports MQTT 3.1.1 packets and is based on the Asio networking library. The implementation process is documented in a blog series: "[Build MQTT Broker](https://chnjfan.github.io/tags/build-mqtt-broker/)".

## Project Structure

```
Jianm/
├── CMakeLists.txt          # Top-level CMake build config
├── conf/
│   └── jianm.conf          # Server configuration file
├── src/
│   ├── main.cpp            # Entry point
│   ├── common/             # Common modules (config, logging, utils)
│   ├── net/                # Network layer (Server, Channel, AdminServer)
│   ├── protocol/           # MQTT protocol parsing (packets, messages)
│   └── session/            # Session management
├── test/                   # Tests (GoogleTest-based)
├── thirdparty/
│   └── spdlog/             # Logging library (header-only mode)
└── build/                  # Build output directory
```

## Build

### Install Dependencies

| Dependency    | Version | Notes                                    |
| ------------- | ------- | ---------------------------------------- |
| CMake         | ≥ 3.16  | Build system                             |
| Asio          | ≥ 1.18  | Cross-platform C++ networking (standalone)|
| C++ Compiler  | C++17   | C++17-capable compiler                   |
| spdlog        | —       | Logging library (bundled, no install)    |
| GoogleTest    | v1.14.0 | Testing framework (only for tests)       |

#### macOS

```bash
# Install CMake and Asio
brew install cmake asio

# C++ compiler: install Xcode Command Line Tools (prompts automatically on first use)
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

> **spdlog** is bundled in the project's `thirdparty/` directory — no installation needed.  
> **GoogleTest** is automatically downloaded by CMake when building tests — no manual installation needed.

### Quick Start

```bash
cd Jianm
cmake -S . -B build
cmake --build build
```

After the build completes, the executable and config file are in `build/bin/`:

```
build/bin/
├── jianm          # MQTT broker executable
└── jianm.conf     # Config file copy
```

### Build Options

| Option              | Default | Notes                                |
| ------------------- | ------- | ------------------------------------ |
| `CMAKE_BUILD_TYPE`  | Release | Build type (Debug/Release/...)       |
| `JIANM_BUILD_TESTS` | ON      | Whether to build the test suite      |

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

Or run the test binary directly:

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
- **10000** — Admin console port (telnet)

### Connect an MQTT Client

Use any MQTT client tool (e.g. `mosquitto_pub`/`mosquitto_sub`):

```bash
# Subscribe to a topic
mosquitto_sub -h localhost -p 1883 -t "test/topic"

# Publish a message
mosquitto_pub -h localhost -p 1883 -t "test/topic" -m "Hello Jianm"
```

### Admin Console

Connect via telnet:

```bash
telnet localhost 10000
```

## Configuration

Server configuration is managed through `conf/jianm.conf`. It is automatically copied to the executable's directory during the build.

```ini
# Log level (trace / debug / info / warn / error / critical)
log_level = trace

# MQTT server port (default 1883)
port = 1883

# Admin console port (default 10000)
admin_port = 10000

# Per-connection receive buffer size in bytes (default 1024)
max_receive_size = 1024

# Allow anonymous connections (true = allowed)
allow_anonymous = true
```

If a config key is not set, the built-in default is used. Restart the server after modifying the config.

## Protocol Support

Based on the MQTT 3.1.1 protocol (OASIS Standard). Current implementation progress:

### Control Packets

| Packet Type | Direction | Status | Description |
| ----------- | --------- | :----: | ----------- |
| CONNECT     | Client → Server | ✅ | Client connection request |
| CONNACK     | Server → Client | ✅ | Connection acknowledgment |
| PUBLISH     | Client ↔ Server | ❌ | Publish message |
| PUBACK      | Client ↔ Server | ❌ | QoS 1 publish acknowledgment |
| PUBREC      | Client ↔ Server | ❌ | QoS 2 publish received |
| PUBREL      | Client ↔ Server | ❌ | QoS 2 publish release |
| PUBCOMP     | Client ↔ Server | ❌ | QoS 2 publish complete |
| SUBSCRIBE   | Client → Server | ❌ | Subscribe to topic |
| SUBACK      | Server → Client | ❌ | Subscribe acknowledgment |
| UNSUBSCRIBE | Client → Server | ❌ | Unsubscribe from topic |
| UNSUBACK    | Server → Client | ❌ | Unsubscribe acknowledgment |
| PINGREQ     | Client → Server | ❌ | Ping request |
| PINGRESP    | Server → Client | ❌ | Ping response |
| DISCONNECT  | Client → Server | ❌ | Disconnect |

> ✅ Implemented　❌ Not yet implemented

### Connection Features

| Feature | Status | Description |
| ------- | :----: | ----------- |
| Clean Session | ✅ | Clean session flag, controls session state clearing and restoration |
| Session Present | ✅ | Correctly calculated based on CleanSession and existing session state |
| Will Flag | ⚠️ | CONNECT packet parsing and validation supported, will message publishing not implemented |
| Username/Password | ⚠️ | CONNECT packet parsing, UTF-8 validation, allow_anonymous config supported; authentication is a stub (always returns true) |
| Keep Alive | ✅ | Automatic disconnect at 1.5× KeepAlive timeout |
| ClientID Validation | ✅ | Length ≤ 23 characters, UTF-8 encoding validation |
| UTF-8 Validation | ✅ | UTF-8 encoding validation for ClientID, Username, and Will Topic |
| Duplicate ClientID | ✅ | Old connection disconnected when same ClientID reconnects (MQTT-3.1.4-2) |
| Duplicate CONNECT | ✅ | Second CONNECT on same connection treated as protocol violation (MQTT-3.1.0-2) |
| Admin Console | ✅ | Telnet on port 10000, supports help/status/sessions/kick/quit commands |
| Log Output | ✅ | Console and file output targets, configurable log level |

> ✅ Implemented　⚠️ Partially implemented　❌ Not implemented

### Messaging & Subscriptions (Not yet implemented)

| Module | Feature | Status | Description |
| ------ | ------- | :----: | ----------- |
| Publish | PUBLISH parsing & routing | ❌ | Route published messages to subscribers |
| Publish | Retained messages | ❌ | Store last retained message for new subscribers |
| Publish | QoS 1 flow | ❌ | AT_LEAST_ONCE delivery + PUBACK |
| Publish | QoS 2 flow | ❌ | EXACTLY_ONCE four-step handshake |
| Subscribe | SUBSCRIBE/SUBACK | ❌ | Subscribe to topics + granted QoS return |
| Subscribe | UNSUBSCRIBE/UNSUBACK | ❌ | Unsubscribe from topics |
| Subscribe | Topic wildcard matching | ❌ | `+` single-level, `#` multi-level wildcards |
| Session | Session state persistence | ❌ | Persist subscriptions and pending messages for CleanSession=0 |
| Session | Offline message queue | ❌ | Buffer offline QoS 1/2 messages for CleanSession=0 |
| Heartbeat | PINGREQ/PINGRESP | ❌ | Client ping request and server response |
| Disconnect | DISCONNECT handling | ❌ | Graceful client disconnect |
| Will | Will Message delivery | ❌ | Publish will message to subscribers on abnormal disconnect |
| Auth | Real authentication | ❌ | Currently a stub (always returns true) |
