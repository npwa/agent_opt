"""
harness/replay.py

Replay mode (F15): re-derive every analysis result of a *recorded* run without a GPU, a model,
OpenCode or Ollama. The recorded files are the only input:

    trials/<id>/record.json (+ events.jsonl, artifacts/)   proxy/metrics.jsonl   gpu_samples.csv
    plan.json   platform.yaml

Pipeline (each step is the same code the live run uses):
  1. verifier replay   - rebuild every trial's working directory from the task fixture plus the
                         recorded artifacts and re-run the task's verifiers on the recorded final
                         answer; the replayed verdict must equal the recorded one
  2. trace join        - harness.trace (events + proxy records + GPU samples)
  3. parity check      - analysis.parity (if the run has uninstrumented trials)
  4. report            - analysis.report (RESULTS.md, report.html, summary.json)

All outputs are deterministic; ``SHA256SUMS`` lists their hashes so a replay can be compared with
a committed golden copy (``diff -r`` / ``cmp SHA256SUMS``).

    python -m harness.replay --run fixtures/replay/parity_pilot --out /tmp/replay
"""

import argparse
import hashlib
import json
import shutil
import sys
import tempfile
from pathlib import Path

from analysis import parity, report
from harness import trace
from harness.taskspec import TaskSpec, load_all
from harness.verifiers import VerifyContext, run_verifiers
from harness.workdir import prepare_workdir

ROOT = Path(__file__).resolve().parent.parent
OUTPUTS = ("RESULTS.md", "report.html", "summary.json", "parity.json", "parity.md", "replay_verdicts.json")


def replay_verdict(spec: TaskSpec, trial_dir: Path, record: dict, fixtures: Path,
                   verify_root: Path) -> dict:
    """Re-run the verifiers of one recorded trial; returns {recorded, replayed, match | skipped}."""
    recorded = record.get("verdict_passed")
    if recorded is None:
        return {"skipped": f"no recorded verdict (status {record.get('status')})"}
    art = trial_dir / "artifacts"
    missing = [f for f in record.get("files", []) if not (art / f).is_file()]
    if missing:
        return {"skipped": f"artifacts not recorded: {missing}"}
    if "comm-tools" in spec.mcp and not (art / "comm_state").is_dir():
        return {"skipped": "comm-tools state was not recorded for this trial"}
    with tempfile.TemporaryDirectory(prefix="agent_opt_replay_") as d:
        work = prepare_workdir(spec, Path(d) / "work", fixtures, verify_root)
        for f in record.get("files", []):
            dest = work / f
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(art / f, dest)
        state = art / "comm_state" if (art / "comm_state").is_dir() else None
        ctx = VerifyContext(workdir=work, verify_root=verify_root, spec=spec,
                            final_answer=record.get("final_answer", ""), state_dir=state)
        verdict = run_verifiers(spec, ctx)
    return {"recorded": recorded, "replayed": verdict.passed, "match": recorded == verdict.passed}


def replay(run_dir: Path, out_dir: Path, tasks_dir: Path = ROOT / "tasks",
           fixtures: Path = ROOT / "fixtures/tasks", verify_root: Path = ROOT / "fixtures/verify",
           verify: bool = True, baseline: str | None = None) -> dict:
    run_dir, out_dir = Path(run_dir), Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    specs = {s.id: s for s in load_all(tasks_dir)}
    verdicts: dict[str, dict] = {}
    with tempfile.TemporaryDirectory(prefix="agent_opt_replay_run_") as tmp:
        work_run = Path(tmp) / run_dir.name
        shutil.copytree(run_dir, work_run)  # never write into the recorded run
        for rec_path in sorted((work_run / "trials").glob("*/record.json")):
            rec = json.loads(rec_path.read_text())
            if verify and rec["task"] in specs:
                verdicts[rec["trial_id"]] = replay_verdict(specs[rec["task"]], rec_path.parent, rec,
                                                           fixtures, verify_root)
        trace.build_all(work_run)
        records = parity.load_records(work_run)
        if any(not r.get("instrumented", True) for r in records):
            res = parity.parity_check(records)
            (work_run / "parity.json").write_text(json.dumps(res, indent=2) + "\n")
            (out_dir / "parity.json").write_text(json.dumps(res, indent=2) + "\n")
            (out_dir / "parity.md").write_text(parity.to_markdown(res))
        summary = report.write_report(work_run, out_dir, tasks_dir, baseline)
    checked = {k: v for k, v in verdicts.items() if "match" in v}
    result = {
        "trials": len(verdicts), "verified": len(checked),
        "mismatches": sorted(k for k, v in checked.items() if not v["match"]),
        "skipped": {k: v["skipped"] for k, v in sorted(verdicts.items()) if "skipped" in v},
        "verdicts": {k: verdicts[k] for k in sorted(verdicts)},
    }
    (out_dir / "replay_verdicts.json").write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    write_hashes(out_dir)
    return {"summary": summary, "replay": result}


def write_hashes(out_dir: Path) -> None:
    lines = [f"{hashlib.sha256((out_dir / n).read_bytes()).hexdigest()}  {n}"
             for n in OUTPUTS if (out_dir / n).exists()]
    (out_dir / "SHA256SUMS").write_text("\n".join(lines) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(description="Re-derive all analysis results from a recorded run")
    ap.add_argument("--run", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--baseline")
    ap.add_argument("--no-verify", action="store_true", help="skip the verifier replay")
    a = ap.parse_args()
    res = replay(a.run, a.out, verify=not a.no_verify, baseline=a.baseline)
    r = res["replay"]
    print(f"replayed {a.run.name}: {res['summary']['trials']['measured']} measured trials, "
          f"{r['verified']}/{r['trials']} verdicts re-verified, {len(r['mismatches'])} mismatches, "
          f"{len(r['skipped'])} skipped -> {a.out}")
    for k in r["mismatches"]:
        print(f"  MISMATCH {k}: recorded {r['verdicts'][k]['recorded']} replayed {r['verdicts'][k]['replayed']}")
    return 1 if r["mismatches"] else 0


if __name__ == "__main__":
    sys.exit(main())
