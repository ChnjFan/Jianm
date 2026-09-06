"""
Jianm MQTT Broker — Integration Test Fixtures

Manages broker lifecycle and provides reusable MQTT clients for test cases.
"""

import os
import socket
import subprocess
import time
import signal
import pytest
import paho.mqtt.client as mqtt


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _find_free_port() -> int:
    """Pick an unused localhost port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def _broker_binary() -> str:
    """Locate the jianm executable.

    Search order:
      1. JIANM_BIN environment variable
       2. build/bin/jianm  (project build directory)
    """
    env_bin = os.environ.get("JIANM_BIN")
    if env_bin:
        return env_bin

    project_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    candidate = os.path.join(project_root, "build", "bin", "jianm")
    return candidate


def _write_config(port: int, admin_port: int, tmpdir: str) -> str:
    """Generate a minimal jianm.conf for the test broker.

    Broker reads config from current working directory, so the file must live
    in the directory we ``Popen`` from.
    """
    conf = os.path.join(tmpdir, "jianm.conf")
    with open(conf, "w") as f:
        f.write(f"port = {port}\n")
        f.write(f"admin_port = {admin_port}\n")
        f.write("allow_anonymous = true\n")
        f.write("max_connections = 64\n")
        f.write("log_level = warn\n")
    return conf


def _wait_for_port(port: int, timeout: float = 10.0) -> bool:
    """Block until the TCP port accepts connections."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.5):
                return True
        except OSError:
            time.sleep(0.1)
    return False


# ---------------------------------------------------------------------------
# Broker fixture
# ---------------------------------------------------------------------------

@pytest.fixture(scope="session")
def broker(tmp_path_factory):
    """Start one jianm broker for the entire test session.

    Yields a BrokerHandle with .port and .admin_port attributes.
    The broker is terminated (SIGTERM) at session teardown.
    """
    mqtt_port = _find_free_port()
    admin_port = _find_free_port()
    workdir = tmp_path_factory.mktemp("jianm_broker")
    conf_path = _write_config(mqtt_port, admin_port, str(workdir))

    env = os.environ.copy()

    binary = _broker_binary()
    if not os.path.isfile(binary):
        pytest.fail(
            f"Broker binary not found: {binary}\n"
            "Build the project first (cmake --build build) or set JIANM_BIN."
        )

    # Broker reads jianm.conf from its *current working directory*, so we set
    # cwd to the temp dir where we wrote the config.
    proc = subprocess.Popen(
        [binary],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env=env,
        cwd=str(workdir),
        preexec_fn=os.setsid,  # create a new process group for cleanup
    )

    handle = BrokerHandle(proc, mqtt_port, admin_port)

    if not _wait_for_port(mqtt_port, timeout=15):
        proc.terminate()
        out = proc.stdout.read().decode(errors="replace") if proc.stdout else ""
        pytest.fail(f"Broker failed to start on port {mqtt_port}.\nOutput:\n{out}")

    yield handle

    # Teardown
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        proc.wait(timeout=5)


class BrokerHandle:
    __slots__ = ("proc", "port", "admin_port")

    def __init__(self, proc, port: int, admin_port: int):
        self.proc = proc
        self.port = port
        self.admin_port = admin_port


# ---------------------------------------------------------------------------
# Client fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def client_factory(broker):
    """Factory that creates connected paho-mqtt clients.

    Callbacks (on_connect, on_message, on_disconnect, on_subscribe) are
    passed as keyword arguments and wired up before connect().

    Usage::
        def test_foo(client_factory):
            c = client_factory("my-client")
            c.publish("topic", "hello")

        def test_bar(client_factory):
            c = client_factory("my-client", on_connect=my_handler)
    """
    created = []
    callback_names = ("on_connect", "on_message", "on_disconnect", "on_subscribe")

    def _make(client_id: str = None, clean_session: bool = True, **kwargs):
        if client_id is None:
            import uuid
            client_id = f"test-{uuid.uuid4().hex[:8]}"

        callbacks = {k: kwargs.pop(k) for k in callback_names if k in kwargs}

        c = mqtt.Client(
            client_id=client_id,
            clean_session=clean_session,
            protocol=mqtt.MQTTv311,
        )
        for name, fn in callbacks.items():
            setattr(c, name, fn)

        c.connect("127.0.0.1", broker.port, **kwargs)
        c.loop_start()
        created.append(c)
        return c

    yield _make

    for c in created:
        try:
            c.loop_stop()
            c.disconnect()
        except Exception:
            pass


@pytest.fixture
def subscriber(client_factory):
    """A helper that subscribes to a topic and collects received messages.

    Returns a SubscriberHandle with .messages list and .wait_for() method.
    """
    class SubscriberHandle:
        def __init__(self):
            self.messages = []
            self._client = None
            self._ready = False

        def subscribe(self, topic: str, qos: int = 0, client_id: str = None):
            def on_connect(c, _ud, _flags, rc):
                if rc == 0:
                    c.subscribe(topic, qos)
                    self._ready = True

            def on_message(_c, _ud, msg):
                self.messages.append(msg)

            self._client = client_factory(
                client_id=client_id,
                on_connect=on_connect,
                on_message=on_message,
            )

        def wait_for(self, timeout: float = 5.0):
            """Wait until the subscription is established."""
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline and not self._ready:
                time.sleep(0.05)
            return self._ready

        def wait_messages(self, count: int, timeout: float = 5.0) -> bool:
            """Wait until at least *count* messages have arrived."""
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                if len(self.messages) >= count:
                    return True
                time.sleep(0.05)
            return False

    return SubscriberHandle()
