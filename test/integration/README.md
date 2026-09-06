# Jianm Integration Tests

English | [中文](../../docs/integration-test-zh.md)

End-to-end tests for the Jianm MQTT broker using Python + paho-mqtt.

## Prerequisites

- Python 3.9+
- Built jianm binary at `build/bin/jianm` (or set `JIANM_BIN`)

## Install

```bash
pip install -r test/integration/requirements.txt
```

## Run

```bash
# From project root (auto-detects build/bin/jianm)
cd test/integration && python -m pytest -v

# Or specify binary explicitly
JIANM_BIN=/path/to/jianm python -m pytest -v

# Run specific test file
python -m pytest test_connect.py -v

# Run specific test
python -m pytest test_connect.py::test_qos0_publish -v
```

## Test Structure

| File | Coverage |
|------|----------|
| `conftest.py` | Broker lifecycle, client factory, subscriber helper |
| `test_connect.py` | CONNECT/CONNACK, Session Present, Will, ClientID validation |
| `test_publish.py` | QoS 0/1/2, Retain, Wildcards, Overlapping dedup |
| `test_subscribe.py` | SUBSCRIBE/UNSUBACK, Granted QoS, Invalid filters |
| `test_session.py` | Offline queue, Subscription persistence, CleanSession |

## How It Works

1. `conftest.py` starts one broker instance per test session (`broker` fixture)
2. Each test gets fresh clients via `client_factory`
3. `subscriber` helper collects messages for assertion
4. Broker is terminated at session teardown
