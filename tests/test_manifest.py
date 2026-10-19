import yaml

from harness import manifest


def test_gpu_info_parses_nvidia_smi(monkeypatch):
    def fake_run(cmd):
        if "--query-gpu=name,driver_version,memory.total" in cmd:
            return "NVIDIA GeForce RTX 3080, 580.126.09, 10240"
        return "| NVIDIA-SMI 580 Driver Version: 580.126.09   CUDA Version: 13.0 |"

    monkeypatch.setattr(manifest, "_run", fake_run)
    assert manifest.gpu_info() == {
        "name": "NVIDIA GeForce RTX 3080", "driver": "580.126.09",
        "memory_mb": 10240, "cuda": "13.0"}


def test_everything_fails_soft(monkeypatch):
    monkeypatch.setattr(manifest, "_run", lambda cmd: None)
    monkeypatch.setattr(manifest, "model_digests", lambda base="": {})
    m = manifest.build_manifest()
    assert m["gpu"]["name"] == manifest.UNAVAILABLE
    assert m["ollama"] == manifest.UNAVAILABLE and m["models"] == {}


def test_tool_version_extracts_number(monkeypatch):
    monkeypatch.setattr(manifest, "_run", lambda cmd: "ollama version is 0.32.5")
    assert manifest.tool_version(["ollama", "--version"]) == "0.32.5"


def test_write_manifest_roundtrip(tmp_path):
    m = {"gpu": {"name": "x"}, "models": {"qwen": "abc"}}
    manifest.write_manifest(tmp_path / "platform.yaml", m)
    assert yaml.safe_load((tmp_path / "platform.yaml").read_text()) == m


def test_llama_cpp_info(monkeypatch, tmp_path):
    monkeypatch.setenv("LLAMA_CPP_DIR", str(tmp_path))
    monkeypatch.setattr(manifest, "_run", lambda cmd: "b2f036c0f")
    assert manifest.llama_cpp_info() == {"dir": str(tmp_path), "commit": "b2f036c0f"}
    monkeypatch.setattr(manifest, "_run", lambda cmd: None)
    assert manifest.llama_cpp_info()["commit"] == manifest.UNAVAILABLE
