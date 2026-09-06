"""
Subscription tests — SUBSCRIBE / UNSUBSCRIBE handling.

Covers:
  - Subscribe with granted QoS
  - Subscribe failure (invalid filter → 0x80)
  - Unsubscribe
  - SUBACK return codes
  - Granted QoS capped at subscription request
"""

import socket
import time
import pytest
import paho.mqtt.client as mqtt


def test_subscribe_granted_qos(broker, client_factory):
    """SUBACK returns the granted QoS for each subscription."""
    granted = []

    def on_subscribe(_c, _ud, mid, granted_qos):
        # paho 1.6+ passes granted_qos as list of ints (or QoS namedtuple)
        for q in granted_qos:
            granted.append(q.value if hasattr(q, 'value') else int(q))

    c = client_factory(client_id="sub-qos-test", on_subscribe=on_subscribe)
    c.subscribe([
        ("test/qos0", 0),
        ("test/qos1", 1),
        ("test/qos2", 2),
    ])
    time.sleep(1)
    c.loop_stop()
    c.disconnect()

    assert len(granted) == 3, f"Expected 3 granted QoS, got {granted}"
    assert granted[0] == 0
    assert granted[1] == 1
    assert granted[2] == 2


def test_subscribe_invalid_filter(broker):
    """Invalid topic filter should be rejected with 0x80 in SUBACK.

    paho-mqtt validates filters client-side, so we send a raw SUBSCRIBE
    packet to test the broker's validation.
    """
    import struct

    # Build SUBSCRIBE packet with invalid filter "a/#/b"
    # Fixed header: type=0x82 (SUBSCRIBE), flags=0x02
    # Variable header: Packet ID = 1
    # Payload: Topic filter "a/#/b" with requested QoS=0
    topic_filter = b'a/#/b'
    payload = struct.pack("!H", 1)  # Packet ID
    payload += struct.pack("!H", len(topic_filter)) + topic_filter
    payload += bytes([0])  # Requested QoS
    fixed_header = bytes([0x82, len(payload)])
    packet = fixed_header + payload

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    s.connect(("127.0.0.1", broker.port))

    # First send CONNECT
    variable_header = b'\x00\x04MQTT\x04\x02\x00\x3C'  # clean=1
    client_id = b'test-invalid-filter'
    payload_connect = struct.pack("!H", len(client_id)) + client_id
    remaining_connect = variable_header + payload_connect
    connect_pkt = bytes([0x10, len(remaining_connect)]) + remaining_connect
    s.sendall(connect_pkt)
    s.recv(4)  # CONNACK

    # Send SUBSCRIBE
    s.sendall(packet)
    # Read SUBACK: 0x90 <len> <pid_hi> <pid_lo> <return_code>
    try:
        resp = s.recv(5)
        if len(resp) >= 5 and resp[0] == 0x90:
            return_code = resp[4]
            assert return_code == 0x80, f"Expected 0x80 for invalid filter, got 0x{return_code:02x}"
        else:
            pytest.fail(f"Unexpected SUBACK response: {resp.hex()}")
    except socket.timeout:
        pytest.fail("Timeout waiting for SUBACK")
    s.close()


def test_unsubscribe(broker, client_factory, subscriber):
    """Unsubscribed client stops receiving messages."""
    topic = "test/unsub/basic"
    subscriber.subscribe(topic, qos=0, client_id="unsub-sub")
    assert subscriber.wait_for(timeout=5)

    pub = client_factory(client_id="unsub-pub")
    pub.publish(topic, "before-unsub", qos=0)
    time.sleep(1)
    assert subscriber.wait_messages(1, timeout=5)

    # Unsubscribe
    subscriber._client.unsubscribe(topic)
    time.sleep(0.5)

    # Publish again — should not arrive
    pub.publish(topic, "after-unsub", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    assert len(subscriber.messages) == 1, (
        f"Expected 1 message (before unsub), got {len(subscriber.messages)}"
    )


def test_granted_qos_capped(broker, client_factory):
    """Granted QoS is the minimum of requested and available.

    Since Jianm doesn't enforce per-topic QoS caps, granted == requested.
    This test verifies the basic contract.
    """
    granted = []

    def on_subscribe(_c, _ud, mid, granted_qos):
        for q in granted_qos:
            granted.append(q.value if hasattr(q, 'value') else int(q))

    c = client_factory(client_id="sub-cap-test", on_subscribe=on_subscribe)
    c.subscribe("test/cap", qos=2)
    time.sleep(1)
    c.loop_stop()
    c.disconnect()

    assert granted == [2], f"Expected granted QoS=2, got {granted}"


def test_multiple_subscribers(broker, client_factory):
    """Multiple subscribers to the same topic all receive the message."""
    topic = "test/multi/sub"
    received = {"A": [], "B": []}

    def make_handler(name):
        def on_message(_c, _ud, msg):
            received[name].append(msg.payload.decode())
        return on_message

    def make_on_connect(name):
        def on_connect(c, _ud, _flags, rc):
            if rc == 0:
                c.subscribe(topic, qos=0)
        return on_connect

    cA = client_factory(client_id="multi-sub-A", on_connect=make_on_connect("A"), on_message=make_handler("A"))
    cB = client_factory(client_id="multi-sub-B", on_connect=make_on_connect("B"), on_message=make_handler("B"))
    time.sleep(1)

    pub = client_factory(client_id="multi-pub")
    pub.publish(topic, "broadcast", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()

    cA.loop_stop()
    cA.disconnect()
    cB.loop_stop()
    cB.disconnect()

    assert received["A"] == ["broadcast"], f"Subscriber A got {received['A']}"
    assert received["B"] == ["broadcast"], f"Subscriber B got {received['B']}"
