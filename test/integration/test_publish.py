"""
Publish tests — PUBLISH packet handling at QoS 0/1/2.

Covers:
  - QoS 0 fire-and-forget
  - QoS 1 publish + PUBACK
  - QoS 2 publish + PUBREC/PUBREL/PUBCOMP handshake
  - Retained message storage and delivery
  - Topic wildcard matching (+ and #)
  - Overlapping subscription deduplication (MQTT-3.3.4)
"""

import time
import pytest
import paho.mqtt.client as mqtt


# ---------------------------------------------------------------------------
# QoS levels
# ---------------------------------------------------------------------------

def test_qos0_publish(broker, client_factory, subscriber):
    """QoS 0: message delivered without acknowledgment."""
    topic = "test/qos0/basic"
    subscriber.subscribe(topic, qos=0, client_id="qos0-sub")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="qos0-pub")
    pub.publish(topic, "hello-qos0", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert subscriber.wait_messages(1, timeout=5)
    assert subscriber.messages[0].payload.decode() == "hello-qos0"
    assert subscriber.messages[0].qos == 0


def test_qos1_publish(broker, client_factory, subscriber):
    """QoS 1: message delivered at least once with PUBACK."""
    topic = "test/qos1/basic"
    subscriber.subscribe(topic, qos=1, client_id="qos1-sub")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="qos1-pub")
    info = pub.publish(topic, "hello-qos1", qos=1)
    info.wait_for_publish(timeout=5)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert subscriber.wait_messages(1, timeout=5)
    assert subscriber.messages[0].payload.decode() == "hello-qos1"
    assert subscriber.messages[0].qos == 1


def test_qos2_publish(broker, client_factory, subscriber):
    """QoS 2: exactly-once four-step handshake."""
    topic = "test/qos2/basic"
    subscriber.subscribe(topic, qos=2, client_id="qos2-sub")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="qos2-pub")
    info = pub.publish(topic, "hello-qos2", qos=2)
    info.wait_for_publish(timeout=5)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert subscriber.wait_messages(1, timeout=5)
    assert subscriber.messages[0].payload.decode() == "hello-qos2"
    assert subscriber.messages[0].qos == 2


# ---------------------------------------------------------------------------
# Retained messages
# ---------------------------------------------------------------------------

def test_retained_message(broker, client_factory, subscriber):
    """Retained message is delivered to new subscribers."""
    topic = "test/retain/basic"

    # Publish a retained message
    pub = client_factory(client_id="retain-pub")
    pub.publish(topic, "retained-payload", qos=1, retain=True)
    time.sleep(0.5)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    # New subscriber should immediately receive the retained message
    subscriber.subscribe(topic, qos=1, client_id="retain-sub")
    assert subscriber.wait_for(timeout=5)
    assert subscriber.wait_messages(1, timeout=5)
    assert subscriber.messages[0].payload.decode() == "retained-payload"
    assert subscriber.messages[0].retain


def test_clear_retained_with_empty_payload(broker, client_factory, subscriber):
    """Publishing empty payload to a retained topic clears it."""
    topic = "retain/clear"

    pub = client_factory(client_id="retain-clear-pub")
    pub.publish(topic, "keep-me", qos=1, retain=True)
    time.sleep(0.5)

    # Clear with empty payload
    pub.publish(topic, "", qos=1, retain=True)
    time.sleep(0.5)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    # New subscriber should NOT receive anything
    subscriber.subscribe(topic, qos=1, client_id="retain-clear-sub")
    assert subscriber.wait_for(timeout=5)
    time.sleep(1)

    # Filter out any messages that might have arrived before clear
    retained = [m for m in subscriber.messages if m.retain]
    assert len(retained) == 0, "Retained message should have been cleared"


# ---------------------------------------------------------------------------
# Topic matching
# ---------------------------------------------------------------------------

def test_single_level_wildcard(broker, client_factory, subscriber):
    """Single-level wildcard '+' matches exactly one level."""
    subscriber.subscribe("sensors/+/temperature", qos=0, client_id="wildcard-sub1")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="wildcard-pub1")
    pub.publish("sensors/room1/temperature", "22.5", qos=0)
    pub.publish("sensors/room2/temperature", "23.0", qos=0)
    # This should NOT match (two levels where + is)
    pub.publish("sensors/room1/humidity", "60", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert subscriber.wait_messages(2, timeout=5)
    payloads = {m.payload.decode() for m in subscriber.messages}
    assert payloads == {"22.5", "23.0"}


def test_multi_level_wildcard(broker, client_factory, subscriber):
    """Multi-level wildcard '#' matches all subsequent levels."""
    subscriber.subscribe("sensors/#", qos=0, client_id="wildcard-sub2")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="wildcard-pub2")
    pub.publish("sensors/room1/temperature", "22.5", qos=0)
    pub.publish("sensors/room2/humidity", "60", qos=0)
    pub.publish("sensors/building1/floor2/room3/light", "on", qos=0)
    # This should NOT match (different root)
    pub.publish("actuators/room1/switch", "on", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert subscriber.wait_messages(3, timeout=5)
    payloads = {m.payload.decode() for m in subscriber.messages}
    assert payloads == {"22.5", "60", "on"}


# ---------------------------------------------------------------------------
# Overlapping subscription deduplication [MQTT-3.3.4]
# ---------------------------------------------------------------------------

def test_overlapping_subscription_dedup(broker, client_factory):
    """When multiple filters match, deliver once at highest QoS.

    Uses an isolated topic namespace to avoid receiving retained messages
    from previous tests (which would inflate the receive count).
    """
    cid = "overlap-sub-001"
    topic = "dedup/isolated/a/b"

    received = []

    def on_message(_c, _ud, msg):
        # Only count non-retained messages on our test topic
        if not msg.retain and msg.topic == topic:
            received.append(msg.payload.decode())

    def on_connect(c, _ud, _flags, rc):
        if rc == 0:
            c.subscribe([
                ("dedup/isolated/a/b", 1),
                ("dedup/isolated/a/+", 1),
                ("dedup/isolated/#", 2),
            ])

    c = mqtt.Client(client_id=cid, protocol=mqtt.MQTTv311)
    c.on_connect = on_connect
    c.on_message = on_message
    c.connect("127.0.0.1", broker.port)
    c.loop_start()
    time.sleep(1)

    pub = client_factory(client_id="overlap-pub")
    pub.publish(topic, "dedup-test", qos=1)
    time.sleep(2)

    pub.loop_stop()
    pub.disconnect()
    c.loop_stop()
    c.disconnect()

    # Should receive exactly ONE message, not three
    assert len(received) == 1, (
        f"Expected 1 message (deduplicated), got {len(received)}: {received}"
    )
    assert received[0] == "dedup-test"
