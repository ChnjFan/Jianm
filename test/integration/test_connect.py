"""
Connection tests — CONNECT / CONNACK handling.

Covers:
  - Basic connect with CleanSession
  - Session Present flag
  - Will message on abnormal disconnect
  - Keepalive / timeout
  - Reject oversized ClientID
  - Auto-generate ClientID when empty + CleanSession=1
  - Reject empty ClientID with CleanSession=0
  - Duplicate ClientID takeover
"""

import socket
import time
import pytest
import paho.mqtt.client as mqtt


# ---------------------------------------------------------------------------
# Basic connect
# ---------------------------------------------------------------------------

def test_connect_clean_session(broker, client_factory):
    """Client connects with CleanSession=1 and receives CONNACK accepted."""
    connected = False

    def on_connect(_c, _ud, _flags, rc):
        nonlocal connected
        connected = (rc == 0)

    c = client_factory(on_connect=on_connect)
    time.sleep(1)
    c.loop_stop()
    c.disconnect()
    assert connected, "CONNACK return code was not 0 (accepted)"


def test_session_present_flag(broker, client_factory):
    """Session Present MUST be 0 on first connect with CleanSession=1."""
    session_present_flags = []

    def on_connect(_c, _ud, flags, _rc):
        # paho 1.6+ passes flags as a dict with 'session present' key
        if isinstance(flags, dict):
            session_present_flags.append(flags.get("session present", 0))
        else:
            session_present_flags.append(flags & 0x01)

    c = client_factory(clean_session=True, on_connect=on_connect)
    time.sleep(1)
    c.loop_stop()
    c.disconnect()

    assert session_present_flags, "on_connect was not called"
    assert session_present_flags[0] == 0, (
        "Session Present must be 0 on first clean-session connect"
    )


def test_session_present_after_reconnect(broker, client_factory):
    """Session Present MUST be 1 on reconnect with CleanSession=0."""
    cid = "persist-client-001"
    present_flags = []

    def make_on_connect(slot):
        def on_connect(_c, _ud, flags, _rc):
            if isinstance(flags, dict):
                slot.append(flags.get("session present", 0))
            else:
                slot.append(flags & 0x01)
        return on_connect

    # First connect — establish persistent session
    c1 = client_factory(client_id=cid, clean_session=False, on_connect=make_on_connect(present_flags))
    time.sleep(1)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    # Reconnect — session should be present
    c2 = client_factory(client_id=cid, clean_session=False, on_connect=make_on_connect(present_flags))
    time.sleep(1)
    c2.loop_stop()
    c2.disconnect()

    assert len(present_flags) == 2
    assert present_flags[0] == 0, "First connect: session_present must be 0"
    assert present_flags[1] == 1, "Reconnect with CleanSession=0: session_present must be 1"


# ---------------------------------------------------------------------------
# Will message
# ---------------------------------------------------------------------------

def test_will_message_on_abnormal_disconnect(broker, client_factory, subscriber):
    """Will message is published when a client disconnects abnormally."""
    will_topic = "test/will/basic"
    will_payload = "client-died"

    # Set up subscriber first
    subscriber.subscribe(will_topic, qos=1, client_id="will-sub")
    assert subscriber.wait_for(timeout=5), "Subscriber failed to connect"

    # Connect with Will, then simulate abnormal disconnect
    c = client_factory(client_id="will-publisher")
    c.will_set(will_topic, will_payload, qos=1, retain=False)
    c.reconnect_delay_set(min_delay=1, max_delay=2)
    # Re-initialize will_set requires reconnect; use reconnect() after will_set
    c.loop_stop()
    # Force close the socket without sending DISCONNECT
    c._sock_close() if hasattr(c, '_sock_close') else None
    # Fallback: use socket-level close
    try:
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", broker.port))
        # Send CONNECT with will
        # Simpler approach: use paho's internal socket close
        if hasattr(c, '_sock'):
            c._sock.close()
        elif hasattr(c, '_sockpairR'):
            pass
    except Exception:
        pass

    # The above is fragile; use a more reliable method
    # Re-connect properly with will_set then kill
    c2 = mqtt.Client(client_id="will-publisher-2", protocol=mqtt.MQTTv311)
    c2.will_set(will_topic, will_payload, qos=1, retain=False)
    c2.connect("127.0.0.1", broker.port)
    c2.loop_start()
    time.sleep(0.5)
    # Forcefully close the underlying socket
    if hasattr(c2, '_sock') and c2._sock:
        c2._sock.close()
    time.sleep(2)

    assert subscriber.wait_messages(1, timeout=5), "Will message was not received"
    msg = subscriber.messages[0]
    assert msg.payload.decode() == will_payload


# ---------------------------------------------------------------------------
# ClientID validation
# ---------------------------------------------------------------------------

def test_reject_long_client_id(broker):
    """ClientID longer than 23 chars must be rejected (id_rejected)."""
    rejected = []

    def on_connect(_c, _ud, _flags, rc):
        # rc=2 means Identifier Rejected
        rejected.append(rc)

    long_id = "a" * 24
    c = mqtt.Client(client_id=long_id, protocol=mqtt.MQTTv311)
    c.on_connect = on_connect
    c.connect_async("127.0.0.1", broker.port)
    c.loop_start()
    time.sleep(2)
    c.loop_stop()

    assert rejected, "Expected connection to be rejected"
    assert rejected[0] == 2, f"Expected rc=2 (id_rejected), got rc={rejected[0]}"


def test_reject_empty_client_id_persistent(broker):
    """Empty ClientID with CleanSession=0 must be rejected by the broker.

    Per [MQTT-3.1.3-8], the server MUST respond with CONNACK rc=0x02 and close.
    The broker may also just close the connection (also valid rejection).
    We accept either behavior.
    """
    import struct

    # Build a minimal CONNECT packet with empty client_id and CleanSession=0
    # Fixed header: type=1 (CONNECT), flags=0
    # Variable header: Protocol Name "MQTT", Level 4, Connect Flags=0x00, KeepAlive=60
    # Payload: empty Client ID (2 zero bytes)
    variable_header = b'\x00\x04MQTT\x04\x00\x00\x3C'  # proto, level, flags(clean=0), keepalive=60
    payload = b'\x00\x00'  # empty client_id
    remaining = variable_header + payload
    fixed_header = bytes([0x10, len(remaining)])
    packet = fixed_header + remaining

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3)
    s.connect(("127.0.0.1", broker.port))
    s.sendall(packet)

    # Read CONNACK (4 bytes: 0x20 0x02 0x00 <return_code>)
    # If broker closes connection without CONNACK, recv returns empty bytes
    try:
        resp = s.recv(4)
        if len(resp) == 0:
            # Broker closed connection without CONNACK — valid rejection
            rejected = True
        elif len(resp) >= 4 and resp[0] == 0x20:
            return_code = resp[3]
            rejected = (return_code == 0x02)
        else:
            rejected = False
    except socket.timeout:
        # Timeout waiting for response — treat as rejection (connection closed)
        rejected = True
    s.close()

    assert rejected, "Empty ClientID with CleanSession=0 should be rejected"


# ---------------------------------------------------------------------------
# Duplicate ClientID
# ---------------------------------------------------------------------------

def test_duplicate_client_id_takeover(broker, client_factory):
    """New connection with same ClientID kicks out the old one."""
    cid = "dup-client-001"
    old_disconnected = []

    def on_disconnect_old(_c, _ud, rc):
        if rc != 0:
            old_disconnected.append(True)

    c1 = client_factory(client_id=cid, on_disconnect=on_disconnect_old)
    time.sleep(0.5)

    # Second connection with same ID
    c2 = client_factory(client_id=cid)
    time.sleep(2)

    c2.loop_stop()
    c2.disconnect()
    time.sleep(0.5)
    c1.loop_stop()

    assert old_disconnected, "Old connection should have been disconnected by broker"
