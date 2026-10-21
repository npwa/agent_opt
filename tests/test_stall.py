import subprocess
import sys
import time

from harness import calibrate, stall


def _proc(code: str):
    return subprocess.Popen([sys.executable, "-c", code], stdout=subprocess.DEVNULL)


def test_silent_idle_process_is_stalled(tmp_path, monkeypatch):
    monkeypatch.setattr(calibrate, "POLL_S", 0.2)
    ev = tmp_path / "events.jsonl"
    ev.write_text("")
    p = _proc("import time; time.sleep(30)")
    try:
        t0 = time.monotonic()
        assert calibrate._wait(p, 60, ev, 1.0, lambda pid: False) == "stalled"
        assert time.monotonic() - t0 < 10
    finally:
        p.kill()


def test_activity_keeps_a_trial_alive(tmp_path, monkeypatch):
    monkeypatch.setattr(calibrate, "POLL_S", 0.2)
    ev = tmp_path / "events.jsonl"
    ev.write_text("")
    # busy GPU/child (probe says so) for a quiet 3 s process: not stalled
    assert calibrate._wait(_proc("import time; time.sleep(3)"), 60, ev, 1.0, lambda pid: True) == 0
    # events keep arriving
    code = f"import time\nfor i in range(10):\n    open({str(ev)!r}, 'a').write('x'); time.sleep(0.3)"
    assert calibrate._wait(_proc(code), 60, ev, 1.0, lambda pid: False) == 0


def test_timeout_still_wins_and_disabled_detector_waits(tmp_path, monkeypatch):
    monkeypatch.setattr(calibrate, "POLL_S", 0.2)
    ev = tmp_path / "events.jsonl"
    ev.write_text("")
    p = _proc("import time; time.sleep(30)")
    try:
        assert calibrate._wait(p, 1.0, ev, None, None) == "timeout"
        assert calibrate._wait(p, 1.0, ev, 0.2, lambda pid: False) == "stalled"
    finally:
        p.kill()


def test_children_busy_sees_a_working_child():
    p = _proc("import time\nt=time.time()\nwhile time.time()-t<4: pass")
    try:
        time.sleep(0.3)
        stall.children_busy(p.pid)  # primes psutil's cpu counters (no children of p: returns False)
        parent = subprocess.Popen([sys.executable, "-c",
                                   "import subprocess,sys,time\n"
                                   "subprocess.Popen([sys.executable,'-c','import time\\nt=time.time()\\nwhile time.time()-t<4: pass'])\n"
                                   "time.sleep(4)"])
        time.sleep(0.5)
        stall.children_busy(parent.pid)
        time.sleep(1.0)
        assert stall.children_busy(parent.pid) is True
        parent.kill()
    finally:
        p.kill()
