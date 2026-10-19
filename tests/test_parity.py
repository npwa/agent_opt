import json
import random

import pytest
from scipy import stats as sps

from analysis import parity
from analysis import stats as astats
from harness import runner, trace
from tests.test_runner import CFG, FIZZ, ROOT, SQRT, specs

# ---- statistics helpers -----------------------------------------------------------------------

@pytest.mark.parametrize(("k", "n"), [(0, 10), (10, 10), (3, 10), (57, 100), (1, 2), (299, 300)])
def test_wilson_matches_scipy(k, n):
    ref = sps.binomtest(k, n).proportion_ci(confidence_level=0.95, method="wilson")
    lo, hi = astats.wilson_interval(k, n)
    assert lo == pytest.approx(ref.low, abs=1e-9) and hi == pytest.approx(ref.high, abs=1e-9)
    assert astats.wilson_interval(0, 0) == (0.0, 1.0)


def test_bootstrap_is_deterministic_and_brackets_mean():
    rnd = random.Random(1)
    data = [rnd.gauss(10, 2) for _ in range(40)]
    a = astats.bootstrap_ci(data, resamples=2000, seed=7)
    assert a == astats.bootstrap_ci(data, resamples=2000, seed=7)
    assert a != astats.bootstrap_ci(data, resamples=2000, seed=8)
    assert a[0] < sum(data) / len(data) < a[1]
    with pytest.raises(ValueError):
        astats.bootstrap_ci([])


def test_bootstrap_coverage_roughly_nominal():
    rnd = random.Random(3)
    hits, sims = 0, 150
    for i in range(sims):
        sample = [rnd.gauss(5.0, 1.0) for _ in range(30)]
        lo, hi = astats.bootstrap_ci(sample, resamples=300, seed=i)
        hits += lo <= 5.0 <= hi
    assert 0.88 <= hits / sims <= 0.99  # the full 1 000-simulation check belongs to M3


def test_geometric_mean():
    assert astats.geometric_mean([1, 4]) == pytest.approx(2.0)


# ---- plan with parity trials --------------------------------------------------------------------

def test_plan_adds_ceil_fraction_of_uninstrumented_trials_per_cell():
    sp = specs(FIZZ, SQRT)
    plan = runner.build_plan([CFG], sp, 10, 1, ROOT / "tasks", parity_fraction=0.2)
    items = plan["order"]
    assert len(items) == 2 * (10 + 2)
    par = [i for i in items if not i["instrumented"]]
    assert len(par) == 4 and all("__p" in i["trial_id"] for i in par)
    assert len({i["trial_id"] for i in items}) == len(items)
    none = runner.build_plan([CFG], sp, 10, 1, ROOT / "tasks")
    assert len(none["order"]) == 20 and all(i["instrumented"] for i in none["order"])
    assert runner.build_plan([CFG], sp, 3, 1, ROOT / "tasks", 0.2)["parity_fraction"] == 0.2
    assert sum(not i["instrumented"] for i in
               runner.build_plan([CFG], sp, 3, 1, ROOT / "tasks", 0.2)["order"]) == 2  # ceil(0.6)=1 each


def test_changing_parity_fraction_is_a_plan_mismatch(tmp_path):
    sp = specs(FIZZ)
    p0 = runner.build_plan([CFG], sp, 5, 1, ROOT / "tasks")
    runner.load_or_create_plan(tmp_path, p0)
    with pytest.raises(runner.PlanMismatchError):
        runner.load_or_create_plan(tmp_path, runner.build_plan([CFG], sp, 5, 1, ROOT / "tasks", 0.2))


def test_run_all_passes_instrumented_flag_and_records_it(tmp_path):
    sp = specs(FIZZ)
    seen = []

    def fn(spec, cfg, tid, attempt, seed, trial_dir, scratch, metrics, instrumented=True):
        seen.append((tid, instrumented))
        return {"status": "pass", "attempt": attempt, "started_utc": "t", "elapsed_s": 1.0}

    out = tmp_path / "out"
    runner.run_all([CFG], sp, 2, 1, out, tmp_path, tmp_path / "m", attempt_fn=fn,
                   log=lambda *_: None, parity_fraction=0.5)
    assert sorted(seen) == [(f"cfg__{FIZZ}__00", True), (f"cfg__{FIZZ}__01", True),
                            (f"cfg__{FIZZ}__p00", False)]
    rec = json.loads((out / "trials" / f"cfg__{FIZZ}__p00" / "record.json").read_text())
    assert rec["instrumented"] is False


def test_e2e_uninstrumented_trial_talks_to_ollama_directly(tmp_path, monkeypatch):
    from tests import test_runner as tr

    # reuse the fake opencode, but make it dump the config it was given
    dump = tmp_path / "cfg_dump.json"
    monkeypatch.setattr(tr, "FAKE", tr.FAKE.replace(
        'mode = os.environ["FAKE_OC_MODE"]',
        'mode = os.environ["FAKE_OC_MODE"]\n'
        'open(os.environ["FAKE_OC_DUMP"], "w").write(open(os.environ["OPENCODE_CONFIG"]).read())'))
    import os
    import stat
    import sys
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    exe = bin_dir / "opencode"
    exe.write_text(tr.FAKE.format(py=sys.executable))
    exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    monkeypatch.setenv("PATH", f"{bin_dir}{os.pathsep}{os.environ['PATH']}")
    monkeypatch.setenv("FAKE_OC_STATE", str(tmp_path / "calls"))
    monkeypatch.setenv("FAKE_OC_GOOD", str(tr.GOLDEN / FIZZ / "good/fizzbuzz.c"))
    monkeypatch.setenv("FAKE_OC_MODE", "ok")
    monkeypatch.setenv("FAKE_OC_DUMP", str(dump))

    sp = specs(FIZZ)[0]
    item = {"trial_id": runner.trial_id("cfg", FIZZ, 0, False), "config": "cfg", "task": FIZZ,
            "k": 0, "instrumented": False}
    rec = runner.run_trial(sp, CFG, item, 5, tmp_path / "out", tmp_path / "scratch",
                           tmp_path / "metrics.jsonl", 0)
    assert rec["status"] == "pass" and rec["instrumented"] is False
    cfg = json.loads(dump.read_text())["provider"]["ollama"]["options"]
    assert cfg["baseURL"] == "http://localhost:11434/v1" and "headers" not in cfg


def test_trace_skips_uninstrumented_trials(tmp_path):
    d = tmp_path / "t"
    d.mkdir()
    (d / "record.json").write_text(json.dumps({"trial_id": "t", "status": "pass", "instrumented": False}))
    assert trace.build_trial_trace(d, tmp_path / "m", None) is None


# ---- the check itself -------------------------------------------------------------------------------

def rec(cfg, task, status, secs, inst, i):
    return {"config": cfg, "task": task, "status": status, "elapsed_s": secs, "instrumented": inst,
            "trial_id": f"{cfg}_{task}_{inst}_{i}"}


def dataset(parity_factor=1.0, parity_pass=1.0, noise=0.02, n_instr=10, n_par=2, tasks=6, seed=0):
    rnd = random.Random(seed)
    out = []
    for t in range(tasks):
        base = 60.0 + 30 * t
        for i in range(n_instr):
            out.append(rec("c", f"t{t}", "pass" if rnd.random() < 0.8 else "fail",
                           base * rnd.gauss(1, noise), True, i))
        for i in range(n_par):
            ok = rnd.random() < parity_pass
            out.append(rec("c", f"t{t}", "pass" if ok else "fail",
                           base * parity_factor * rnd.gauss(1, noise), False, i))
    return out


def test_equivalent_paths_are_ok():
    res = parity.parity_check(dataset(), resamples=2000)
    assert res["time"]["verdict"] == "ok" and res["verdict"] in ("ok", "inconclusive")
    assert res["time"]["ratio"] == pytest.approx(1.0, abs=0.03)
    assert res["n_pairs"] == 12 and res["success"]["parity"]["n"] == 12


def test_slow_proxy_is_a_time_violation():
    res = parity.parity_check(dataset(parity_factor=0.8), resamples=2000)  # proxy adds 25 %
    assert res["time"]["verdict"] == "violation" and res["verdict"] == "violation"
    assert res["time"]["ci"][1] < 0.95


def test_noisy_small_sample_is_inconclusive_not_a_violation():
    res = parity.parity_check(dataset(parity_factor=1.04, noise=0.25, n_par=1, tasks=6), resamples=2000)
    assert res["time"]["verdict"] == "inconclusive"
    assert res["verdict"] in ("inconclusive", "violation")  # success may differ by chance only


def test_success_rate_difference_is_a_violation():
    res = parity.parity_check(dataset(parity_pass=0.0, n_par=4, tasks=8), resamples=2000)
    assert res["success"]["verdict"] == "violation" and res["verdict"] == "violation"


def test_insufficient_data_is_reported():
    res = parity.parity_check(dataset(n_par=1, tasks=2), resamples=100)
    assert res["verdict"] == "insufficient" and "paired trials" in res["time"]["reason"]
    assert parity.parity_check([], resamples=100)["verdict"] == "insufficient"


def test_parity_trials_without_instrumented_partner_are_ignored():
    data = dataset() + [rec("c", "orphan", "pass", 10, False, 0)]
    res = parity.parity_check(data, resamples=500)
    assert res["parity_without_instrumented_partner"] == 1 and res["n_pairs"] == 12


def test_load_records_excludes_harness_failures(tmp_path):
    for name, st in (("a", "pass"), ("b", "harness_failed"), ("c", "timeout")):
        d = tmp_path / "trials" / name
        d.mkdir(parents=True)
        (d / "record.json").write_text(json.dumps({"status": st}))
    assert sorted(r["status"] for r in parity.load_records(tmp_path)) == ["pass", "timeout"]


def test_cli_writes_reports_and_sets_exit_code(tmp_path, capsys):
    for i, r in enumerate(dataset(parity_factor=0.7)):
        d = tmp_path / "trials" / f"x{i}"
        d.mkdir(parents=True)
        (d / "record.json").write_text(json.dumps(r))
    assert parity.main(["--run", str(tmp_path)]) == 1
    out = capsys.readouterr().out
    assert "VIOLATION" in out and (tmp_path / "parity.json").exists()
    assert "ratio" in (tmp_path / "parity.md").read_text()
