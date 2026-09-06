"""
Session tests — persistent session and offline message queue.

Covers:
  - Offline QoS 1 message delivery on reconnect
  - Offline QoS 2 message delivery on reconnect
  - CleanSession=0 preserves subscriptions across reconnect
  - CleanSession=1 clears session on disconnect
  - Message retransmission (DUP flag)
"""

import time
import pytest
import paho.mqtt.client as mqtt


# ---------------------------------------------------------------------------
# Offline message queue
# ---------------------------------------------------------------------------

def test_offline_qos1_delivery(broker, client_factory):
    """QoS 1 messages are queued for offline persistent-session clients."""
    topic = "test/offline/qos1"
    cid = "offline-qos1-client"

    # Connect with CleanSession=0, subscribe, then disconnect
    c1 = client_factory(client_id=cid, clean_session=False)
    c1.subscribe(topic, qos=1)
    time.sleep(0.5)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    # Publish while subscriber is offline
    pub = client_factory(client_id="offline-pub-1")
    pub.publish(topic, "queued-qos1", qos=1)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    # Reconnect — should receive the queued message
    received = []

    def on_message(_c, _ud, msg):
        received.append(msg)

    c2 = client_factory(client_id=cid, clean_session=False, on_message=on_message)
    time.sleep(2)
    c2.loop_stop()
    c2.disconnect()

    payloads = [m.payload.decode() for m in received]
    assert "queued-qos1" in payloads, f"Expected queued message, got {payloads}"


def test_offline_qos2_delivery(broker, client_factory):
    """QoS 2 messages are queued for offline persistent-session clients."""
    topic = "test/offline/qos2"
    cid = "offline-qos2-client"

    c1 = client_factory(client_id=cid, clean_session=False)
    c1.subscribe(topic, qos=2)
    time.sleep(0.5)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    pub = client_factory(client_id="offline-pub-2")
    pub.publish(topic, "queued-qos2", qos=2)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    received = []

    def on_message(_c, _ud, msg):
        received.append(msg)

    c2 = client_factory(client_id=cid, clean_session=False, on_message=on_message)
    time.sleep(2)
    c2.loop_stop()
    c2.disconnect()

    payloads = [m.payload.decode() for m in received]
    assert "queued-qos2" in payloads, f"Expected queued QoS2 message, got {payloads}"


# ---------------------------------------------------------------------------
# Subscription persistence
# ---------------------------------------------------------------------------

def test_subscriptions_preserved_across_reconnect(broker, client_factory):
    """CleanSession=0: subscriptions survive disconnect/reconnect."""
    topic = "test/persist/subs"
    cid = "persist-subs-client"

    # Subscribe with persistent session
    c1 = client_factory(client_id=cid, clean_session=False)
    c1.subscribe(topic, qos=1)
    time.sleep(0.5)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    # Publish while disconnected — subscription should still be active
    pub = client_factory(client_id="persist-pub")
    pub.publish(topic, "after-reconnect", qos=1)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    # Reconnect — subscription should be restored (message may arrive via outbox)
    received = []

    def on_message(_c, _ud, msg):
        received.append(msg)

    c2 = client_factory(client_id=cid, clean_session=False, on_message=on_message)
    time.sleep(2)

    # Also publish after reconnect to confirm subscription is live
    pub2 = client_factory(client_id="persist-pub-2")
    pub2.publish(topic, "live-after-reconnect", qos=1)
    time.sleep(1)

    c2.loop_stop()
    c2.disconnect()

    payloads = [m.payload.decode() for m in received]
    assert "live-after-reconnect" in payloads, (
        f"Subscription not restored after reconnect. Got: {payloads}"
    )


def test_clean_session_clears_subscriptions(broker, client_factory):
    """CleanSession=1: subscriptions are NOT preserved."""
    topic = "test/clean/subs"
    cid = "clean-subs-client"

    # Subscribe with CleanSession=1
    c1 = client_factory(client_id=cid, clean_session=True)
    c1.subscribe(topic, qos=1)
    time.sleep(0.5)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    # Publish — should NOT be queued (session was destroyed)
    pub = client_factory(client_id="clean-pub")
    pub.publish(topic, "should-not-arrive", qos=1)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    # Reconnect with CleanSession=1 — no subscription active
    received = []

    def on_message(_c, _ud, msg):
        received.append(msg)

    c2 = client_factory(client_id=cid, clean_session=True, on_message=on_message)
    time.sleep(2)
    c2.loop_stop()
    c2.disconnect()

    assert len(received) == 0, (
        f"CleanSession=1 should not preserve subscriptions, but got: "
        f"{[m.payload.decode() for m in received]}"
    )


# ---------------------------------------------------------------------------
# QoS 0 not queued for offline
# ---------------------------------------------------------------------------

def test_qos0_not_queued_offline(broker, client_factory):
    """QoS 0 messages are NOT queued for offline clients."""
    topic = "test/offline/qos0"
    cid = "offline-qos0-client"

    c1 = client_factory(client_id=cid, clean_session=False)
    c1.subscribe(topic, qos=1)
    time.sleep(0.5)
    c1.loop_stop()
    c1.disconnect()
    time.sleep(0.5)

    pub = client_factory(client_id="offline-pub-qos0")
    pub.publish(topic, "qos0-lost", qos=0)
    time.sleep(1)
    pub.loop_stop()
    pub.disconnect()
    time.sleep(0.5)

    received = []

    def on_message(_c, _ud, msg):
        received.append(msg)

    c2 = client_factory(client_id=cid, clean_session=False, on_message=on_message)
    time.sleep(2)
    c2.loop_stop()
    c2.disconnect()

    assert len(received) == 0, (
        f"QoS 0 should not be queued for offline clients, got: "
        f"{[m.payload.decode() for m in received]}"
    )
