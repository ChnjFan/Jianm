# Jianm 集成测试

[English](../test/integration/README.md) | 中文

基于 Python + paho-mqtt 的 Jianm MQTT Broker 端到端测试。

## 前置条件

- Python 3.9+
- 已构建的 jianm 二进制文件（`build/bin/jianm`），或通过 `JIANM_BIN` 环境变量指定

## 安装

```bash
pip install -r test/integration/requirements.txt
```

## 运行

```bash
# 在项目根目录运行（自动检测 build/bin/jianm）
cd test/integration && python -m pytest -v

# 或显式指定二进制文件路径
JIANM_BIN=/path/to/jianm python -m pytest -v

# 运行指定测试文件
python -m pytest test_connect.py -v

# 运行单个测试
python -m pytest test_connect.py::test_qos0_publish -v
```

## 测试结构

| 文件 | 覆盖范围 |
|------|----------|
| `conftest.py` | Broker 生命周期管理、客户端工厂、订阅者辅助工具 |
| `test_connect.py` | CONNECT/CONNACK、Session Will、ClientID 校验 |
| `test_publish.py` | QoS 0/1/2、Retain、通配符匹配、重叠订阅去重 |
| `test_subscribe.py` | SUBSCRIBE/UNSUBACK、Granted QoS、非法 filter 校验 |
| `test_session.py` | 离线消息队列、订阅持久化、CleanSession 语义 |

## 测试用例

### 连接测试（test_connect.py）

| 测试 | 说明 |
|------|------|
| `test_connect_clean_session` | CleanSession=1 连接，CONNACK 返回 accepted |
| `test_session_present_flag` | 首次连接 Session Present 必须为 0 |
| `test_session_present_after_reconnect` | CleanSession=0 重连 Session Present 必须为 1 |
| `test_will_message_on_abnormal_disconnect` | 异常断开时发布 Will 消息 |
| `test_reject_long_client_id` | ClientID 超过 23 字符返回 id_rejected |
| `test_reject_empty_client_id_persistent` | 空 ClientID + CleanSession=0 必须被拒绝 |
| `test_duplicate_client_id_takeover` | 相同 ClientID 重连时断开旧连接 |

### 发布测试（test_publish.py）

| 测试 | 说明 |
|------|------|
| `test_qos0_publish` | QoS 0 消息投递（无确认） |
| `test_qos1_publish` | QoS 1 消息投递 + PUBACK |
| `test_qos2_publish` | QoS 2 四次握手（PUBREC→PUBREL→PUBCOMP） |
| `test_retained_message` | Retained 消息对新订阅者立即投递 |
| `test_clear_retained_with_empty_payload` | 空 payload 清除 Retained 消息 |
| `test_single_level_wildcard` | `+` 单层通配符匹配 |
| `test_multi_level_wildcard` | `#` 多层通配符匹配 |
| `test_overlapping_subscription_dedup` | 重叠订阅去重（[MQTT-3.3.4] 只投递一次，取最高 QoS） |

### 订阅测试（test_subscribe.py）

| 测试 | 说明 |
|------|------|
| `test_subscribe_granted_qos` | SUBACK 返回各订阅的 granted QoS |
| `test_subscribe_invalid_filter` | 非法 topic filter 返回 0x80 |
| `test_unsubscribe` | 取消订阅后不再收到消息 |
| `test_granted_qos_capped` | Granted QoS 不超过请求值 |
| `test_multiple_subscribers` | 多个订阅者均收到消息 |

### 会话测试（test_session.py）

| 测试 | 说明 |
|------|------|
| `test_offline_qos1_delivery` | 离线 QoS 1 消息在重连时投递 |
| `test_offline_qos2_delivery` | 离线 QoS 2 消息在重连时投递 |
| `test_subscriptions_preserved_across_reconnect` | CleanSession=0 订阅跨连接保持 |
| `test_clean_session_clears_subscriptions` | CleanSession=1 断开即清除订阅 |
| `test_qos0_not_queued_offline` | QoS 0 消息不离线缓存 |

## 工作原理

1. `conftest.py` 中的 `broker` fixture 在整个测试会话期间启动一个 Broker 实例
2. `client_factory` fixture 为每个测试用例创建独立的客户端
3. `subscriber` 辅助工具订阅主题并收集收到的消息，便于断言
4. 会话结束时 Broker 自动终止（SIGTERM）

## 配置

测试 Broker 使用独立的临时配置（`conftest.py` 自动生成），避免与默认配置冲突：

```ini
port = <随机端口>
admin_port = <随机端口>
allow_anonymous = true
max_connections = 64
log_level = warn
```

## CI 集成

通过 GitHub Actions 自动运行（`.github/workflows/smoke-test.yml`）：

| 任务 | 说明 |
|------|------|
| `broker-unit` | 构建 + C++ 单元测试（`ctest`） |
| `broker-integration` | 构建 + Python 集成测试（Python 3.9–3.12） |
