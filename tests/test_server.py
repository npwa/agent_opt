import json
import stat
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

import pytest
import yaml

from harness import proxy, runner
from harness import server as srv
from tests.test_runner import CFG, FIZZ, SQRT, specs

FIXTURE = Path(__file__).parent.parent / "fixtures/server/ollama_serve_q8_fa.log"


# ---- settings -----------------------------------------------------------------------------------

def test_validate_server():
    assert srv.validate_server({}) == []
    assert srv.validate_server({"num_ctx": 16384, "kv_cache_type": "q8_0", "flash_attention": True, "num_parallel": 2,
                                "base_model": "qwen3.6"}) == []
    for bad, frag in [({"num_ctx": 10}, "num_ctx"), ({"num_ctx": True}, "num_ctx"), ({"kv_cache_type": "q5"}, "kv_cache_type"),
                      ({"flash_attention": "yes"}, "flash_attention"), ({"num_parallel": 0}, "num_parallel"), ({"fit_target_mib": -1}, "fit_target_mib"),
                      ({"bogus": 1}, "unknown key"), ({"base_model": ""}, "base_model")]:
        errs = srv.validate_server(bad)
        assert errs and any(frag in e for e in errs), (bad, errs)
    assert srv.validate_server([1])[0].startswith("server: must be a mapping")


def test_env_only_sets_what_was_asked_for():
    assert srv.server_env({}) == {"OLLAMA_HOST": srv.PRIVATE_HOST, "OLLAMA_MODELS": srv.DEFAULT_MODELS_DIR}
    e = srv.server_env({"kv_cache_type": "q8_0", "flash_attention": False, "num_parallel": 2, "num_ctx": 8192}, "h:1", "/m")
    assert e == {"OLLAMA_HOST": "h:1", "OLLAMA_MODELS": "/m", "OLLAMA_KV_CACHE_TYPE": "q8_0",
                 "OLLAMA_FLASH_ATTENTION": "0", "OLLAMA_NUM_PARALLEL": "2"}  # num_ctx is the model's, not an env var
    assert srv.server_env({"fit_target_mib": 512})["LLAMA_ARG_FIT_TARGET"] == "512"


def test_group_key_is_stable_and_distinguishes_settings():
    assert srv.group_key(None) == "system"
    assert srv.group_key({"a": 1, "b": 2}) == srv.group_key({"b": 2, "a": 1})
    assert srv.group_key({}) != srv.group_key(None) != srv.group_key({"num_ctx": 8192})


def test_config_with_server_section_loads_and_is_validated(tmp_path):
    good = tmp_path / "c.yaml"
    good.write_text("id: c1\nmodel: ollama/qwen3.6-16k\nserver:\n  num_ctx: 16384\n  kv_cache_type: q8_0\n")
    assert runner.load_config(good).server == {"num_ctx": 16384, "kv_cache_type": "q8_0"}
    bad = tmp_path / "bad.yaml"
    bad.write_text("id: c1\nmodel: ollama/m\nserver:\n  kv_cache_type: q9\n")
    with pytest.raises(ValueError, match="kv_cache_type"):
        runner.load_config(bad)
    assert runner.load_config(Path(__file__).parent.parent / "configs/baseline_32k.yaml").server is None


# ---- reading back the server's log -------------------------------------------------------------------

def test_parse_real_server_log():
    f = srv.parse_server_log(FIXTURE.read_text())
    assert f["llama_server_args"] == {"ctx": 32768, "parallel": 1, "cache_type_k": "q8_0", "cache_type_v": "q8_0",
                                      "flash_attn": "on", "batch": 512, "ubatch": 512}
    assert f["layers_offloaded"] == {"gpu": 41, "total": 42}
    assert f["fit"] == {"layers": 41, "layers_overflowing": 29, "vram_used_mib": 7581, "vram_free_mib": 1991}
    assert f["model_buffer_gpu_mib"] == pytest.approx(6871.01) and f["model_buffer_cpu_mib"] == pytest.approx(15094.36)
    assert f["kv_cache"] == {"mib": 340.0, "cells": 32768, "layers": 10, "k_type": "q8_0", "v_type": "q8_0"}
    assert f["context"] == {"n_ctx": 32768, "flash_attn": "enabled"}
    assert f["fit_free_vram_targets_mib"] == [1901] and f["prompt_cache_limit_mib"] == 8192 and f["context_checkpoints"] == {"max": 32, "min_spacing": 8192}
    assert f["env"]["OLLAMA_KV_CACHE_TYPE"] == "q8_0" and f["env"]["OLLAMA_FLASH_ATTENTION"] == "true"


def test_parse_uses_the_last_load_only_and_tolerates_missing_lines():
    two = FIXTURE.read_text() + FIXTURE.read_text().replace("32768", "16384").replace("q8_0", "f16")
    f = srv.parse_server_log(two)
    assert f["llama_server_args"]["ctx"] == 16384 and f["kv_cache"]["k_type"] == "f16"
    empty = srv.parse_server_log("nothing useful\n")
    assert empty["kv_cache"] is None and empty["layers_offloaded"] is None and empty["llama_server_args"]["ctx"] is None


def test_check_applied_reports_every_mismatch():
    facts = srv.parse_server_log(FIXTURE.read_text())
    assert srv.check_applied({"num_ctx": 32768, "kv_cache_type": "q8_0", "flash_attention": True, "num_parallel": 1}, facts) == []
    probs = srv.check_applied({"num_ctx": 16384, "kv_cache_type": "f16", "flash_attention": False, "num_parallel": 4}, facts)
    assert len(probs) == 4 and any("flash_attention" in p for p in probs)
    assert srv.check_applied({"num_ctx": 1}, {}) == []  # nothing known, nothing to contradict
    assert srv.check_applied({"fit_target_mib": 512}, {"env": {"LLAMA_ARG_FIT_TARGET": "512"}}) == []
    assert "fit_target_mib" in srv.check_applied({"fit_target_mib": 512}, {"env": {"LLAMA_ARG_FIT_TARGET": ""}})[0]


# ---- plan blocks ---------------------------------------------------------------------------------------

def test_plan_runs_server_configs_in_blocks_and_plain_plans_are_unchanged():
    sp = specs(FIZZ, SQRT)
    plain = runner.build_plan([CFG, runner.RunConfig(id="c2", model="ollama/m")], sp, 3, 7, runner.ROOT / "tasks")
    assert [t["config"] for t in plain["order"]] != sorted(t["config"] for t in plain["order"])  # interleaved
    a = runner.RunConfig(id="a", model="ollama/m", server={"num_ctx": 8192})
    b = runner.RunConfig(id="b", model="ollama/m", server={"kv_cache_type": "q8_0"})
    c = runner.RunConfig(id="c", model="ollama/m")
    plan = runner.build_plan([a, b, c], sp, 3, 7, runner.ROOT / "tasks")
    cfgs = [t["config"] for t in plan["order"]]
    blocks = [cfgs[0]] + [x for prev, x in zip(cfgs, cfgs[1:], strict=False) if x != prev]
    assert sorted(blocks) == ["a", "b", "c"] and len(blocks) == 3  # each configuration is one contiguous block
    again = runner.build_plan([a, b, c], sp, 3, 7, runner.ROOT / "tasks")
    assert [t["trial_id"] for t in again["order"]] == [t["trial_id"] for t in plan["order"]]  # seeded
    # same-server configurations share a block and stay shuffled inside it
    d = runner.RunConfig(id="d", model="ollama/m", server={"num_ctx": 8192}, options={"temperature": 0})
    mixed = runner.build_plan([a, d], sp, 3, 7, runner.ROOT / "tasks")
    assert len({t["config"] for t in mixed["order"][:6]}) == 2


# ---- run_all switches servers ---------------------------------------------------------------------------

class FakeManaged:
    events: list = []

    def __init__(self, server, model, log_path):
        self.server, self.model, self.log_path = server, model, log_path
        self.url, self.v1 = "http://127.0.0.1:19999", "http://127.0.0.1:19999/v1"

    def start(self):
        FakeManaged.events.append(("start", self.model, runner.OLLAMA_DIRECT_URL))
        return {"requested": self.server, "model": self.model, "not_as_requested": []}

    def stop(self):
        FakeManaged.events.append(("stop", self.model))


def test_run_all_starts_stops_and_points_the_proxy_and_direct_trials_at_each_server(tmp_path):
    FakeManaged.events = []
    seen = []

    def attempt(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics, instrumented=True):
        up = (metrics.parent / "upstream.txt")
        seen.append((cfg.id, up.read_text() if up.exists() else None, runner.OLLAMA_DIRECT_URL))
        return {"status": "pass", "attempt": attempt, "elapsed_s": 1.0}

    a = runner.RunConfig(id="ctx8k", model="ollama/m8", server={"num_ctx": 8192})
    c = runner.RunConfig(id="plain", model="ollama/m")
    (tmp_path / "proxy").mkdir()
    runner.run_all([a, c], specs(FIZZ), 2, 5, tmp_path / "out", tmp_path / "s", tmp_path / "proxy/metrics.jsonl",
                   attempt_fn=attempt, log=lambda *_: None, server_factory=FakeManaged)
    assert len(seen) == 4
    for cfg_id, upstream, direct in seen:
        if cfg_id == "ctx8k":
            assert upstream == "http://127.0.0.1:19999" and direct == "http://127.0.0.1:19999/v1"
        else:
            assert upstream is None and direct == runner.DEFAULT_DIRECT_URL
    assert [e[0] for e in FakeManaged.events] == ["start", "stop"]
    assert not (tmp_path / "proxy/upstream.txt").exists() and runner.OLLAMA_DIRECT_URL == runner.DEFAULT_DIRECT_URL
    plat = yaml.safe_load((tmp_path / "out/platform.yaml").read_text())
    assert plat["servers"]["ctx8k"]["requested"] == {"num_ctx": 8192} and "plain" not in plat["servers"]


def test_run_all_cleans_up_when_a_trial_raises(tmp_path):
    FakeManaged.events = []

    def boom(*a, **k):
        raise KeyboardInterrupt

    a = runner.RunConfig(id="ctx8k", model="ollama/m8", server={})
    (tmp_path / "proxy").mkdir()
    with pytest.raises(KeyboardInterrupt):
        runner.run_all([a], specs(FIZZ), 1, 5, tmp_path / "out", tmp_path / "s", tmp_path / "proxy/metrics.jsonl",
                       attempt_fn=boom, log=lambda *_: None, server_factory=FakeManaged)
    assert FakeManaged.events[-1][0] == "stop" and not (tmp_path / "proxy/upstream.txt").exists()
    assert runner.OLLAMA_DIRECT_URL == runner.DEFAULT_DIRECT_URL


async def test_proxy_follows_upstream_txt(monkeypatch, tmp_path):
    from tests.test_proxy import SSE, make_wired
    captured = []
    c = make_wired(monkeypatch, tmp_path, [SSE], passthrough=True, captured=captured)
    monkeypatch.setattr(proxy, "OLLAMA_BASE", "http://default:11434")
    await c.post("/v1/chat/completions", json={"model": "m"})
    (tmp_path / "upstream.txt").write_text("http://127.0.0.1:11437/\n")
    await c.post("/v1/chat/completions", json={"model": "m"})
    (tmp_path / "upstream.txt").unlink()
    await c.post("/v1/chat/completions", json={"model": "m"})
    assert [str(r.url) for r in captured] == ["http://default:11434/v1/chat/completions",
                                              "http://127.0.0.1:11437/v1/chat/completions",
                                              "http://default:11434/v1/chat/completions"]


# ---- ManagedServer against fake ollama processes -------------------------------------------------------------

FAKE_OLLAMA = '''#!{py}
import json, sys, threading, os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
host = os.environ["OLLAMA_HOST"]
print(open({fixture!r}).read(), flush=True)
class H(BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def reply(self, obj):
        b = json.dumps(obj).encode(); self.send_response(200); self.send_header("Content-Length", str(len(b))); self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        self.reply({{"models": [{{"name": "m", "size_vram": 8, "context_length": 32768}}]}})
    def do_POST(self):
        self.rfile.read(int(self.headers.get("Content-Length", 0))); self.reply({{"done": True}})
ThreadingHTTPServer((host.split(":")[0], int(host.split(":")[1])), H).serve_forever()
'''


class FakeService(BaseHTTPRequestHandler):
    log: list = []
    loaded = [{"name": "qwen3.6-32k"}]

    def log_message(self, *a):
        pass

    def _reply(self, obj):
        b = json.dumps(obj).encode()
        self.send_response(200)
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_GET(self):
        FakeService.log.append(("GET", self.path))
        self._reply({"models": list(FakeService.loaded)})

    def do_POST(self):
        body = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))) or b"{}")
        FakeService.log.append(("POST", self.path, body))
        if self.path == "/api/generate" and body.get("keep_alive") == 0:
            FakeService.loaded = []
        if self.path == "/api/show":
            self._reply({"parameters": "num_ctx                        32768\n"})
        else:
            self._reply({"status": "success"})


@pytest.fixture
def service():
    FakeService.log, FakeService.loaded = [], [{"name": "qwen3.6-32k"}]
    s = ThreadingHTTPServer(("127.0.0.1", 0), FakeService)
    threading.Thread(target=s.serve_forever, daemon=True).start()
    yield f"http://127.0.0.1:{s.server_address[1]}"
    s.shutdown()


def free_port() -> int:
    import socket
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def test_release_vram_and_ensure_model(service):
    assert srv.release_vram(service, wait_s=5) == ["qwen3.6-32k"]
    assert any(e[0] == "POST" and e[2] == {"model": "qwen3.6-32k", "keep_alive": 0} for e in FakeService.log if len(e) > 2)
    assert srv.ensure_model(service, "qwen3.6-32k", "qwen3.6", 32768) is False  # already has that num_ctx
    assert srv.ensure_model(service, "qwen3.6-8k", "qwen3.6", 8192) is True
    created = [e[2] for e in FakeService.log if e[:2] == ("POST", "/api/create")]
    assert created == [{"model": "qwen3.6-8k", "from": "qwen3.6", "parameters": {"num_ctx": 8192}, "stream": False}]


def test_managed_server_start_reads_back_facts_and_stops(tmp_path, service):
    exe = tmp_path / "ollama"
    exe.write_text(FAKE_OLLAMA.format(py=sys.executable, fixture=str(FIXTURE)))
    exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    host = f"127.0.0.1:{free_port()}"
    m = srv.ManagedServer({"num_ctx": 16384, "kv_cache_type": "f16"}, "qwen3.6-16k", tmp_path / "servers/x.log", host=host,
                          service_url=service, ollama_bin=str(exe))
    with m:
        facts = m.facts
        assert facts["model"] == "qwen3.6-16k" and facts["requested"] == {"num_ctx": 16384, "kv_cache_type": "f16"}
        assert facts["kv_cache"]["k_type"] == "q8_0" and facts["ps"] == {"size_vram_bytes": 8, "context_length": 32768}
        # the fake log says 32768 / q8_0: the mismatch with what was requested is reported, not hidden
        assert len(facts["not_as_requested"]) == 2
        assert srv._get(f"{m.url}/api/tags", 1) is not None
    assert ("POST", "/api/generate") in [e[:2] for e in FakeService.log]  # system model unloaded first
    assert any(e[:2] == ("POST", "/api/create") for e in FakeService.log)  # derived model created for num_ctx
    assert srv._get(f"{m.url}/api/tags", 1) is None  # stopped
    assert (tmp_path / "servers/x.log").exists()


def test_managed_server_refuses_a_busy_port(tmp_path, service):
    m = srv.ManagedServer({}, "m", tmp_path / "l.log", host=service.removeprefix("http://"), service_url=service)
    with pytest.raises(RuntimeError, match="already in use"):
        m.start()


def test_record_server_merges_into_platform_yaml(tmp_path):
    p = tmp_path / "platform.yaml"
    p.write_text(yaml.safe_dump({"gpu": "x"}))
    srv.record_server(p, "a", {"model": "m"})
    srv.record_server(p, "b", {"model": "n"})
    d = yaml.safe_load(p.read_text())
    assert d["gpu"] == "x" and set(d["servers"]) == {"a", "b"}
