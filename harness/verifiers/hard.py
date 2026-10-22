"""
Verifiers of the "hard" tasks (plan item P5): each needs more than a rubric or a unit-test file.

* ``wcx``              compile the agent's ``wcx.c`` with the required flags, forbid dynamic memory, run it on edge-case inputs
* ``harbor_answers``   split a numbered answer into items and check each against the corrected facts of the report
* ``tz_slot``          the meeting booked through the mock comm-tools: right slot, right attendees, one local-time email each

Every function takes the VerifyContext first (see harness/verifiers/__init__.py) and returns a VerifierResult.
"""

import json
import re
from datetime import UTC, datetime
from pathlib import Path

from harness import sandbox
from harness.verifiers import VerifierResult, VerifyContext

# ---- wcx ---------------------------------------------------------------------------------------------------------

WS = b" \t\n\v\f\r"
FORBIDDEN_CALLS = re.compile(r"\b(malloc|calloc|realloc|getline|getdelim|strdup|strndup|mmap|alloca|reallocarray)\b")
CFLAGS = ["-std=c11", "-Wall", "-Wextra", "-Werror", "-O2"]
BIG = 64 * 1024 * 1024


def wcx_reference(data: bytes) -> tuple[int, int, int, int]:
    """lines, words, bytes, maxlen exactly as WCX.md defines them."""
    lines = data.count(b"\n")
    words, in_word = 0, False
    for b in data:
        if b in WS:
            in_word = False
        elif not in_word:
            in_word, words = True, words + 1
    maxlen = max((len(part) for part in data.split(b"\n")), default=0)
    return lines, words, len(data), maxlen


WCX_CASES = {
    "empty": b"",
    "one newline": b"\n",
    "no trailing newline": b"hello world",
    "unterminated last line is the longest": b"a\nbb\nccc dddd",
    "crlf": b"ab cd\r\nef\r\n",
    "tab vtab formfeed": b"a\tb\vc\fd",
    "nul bytes belong to words": b"a\x00b c\x00",
    "high bytes belong to words": b"\xc2\xa0x \xff\xfe\x85\n\xa0\n",
    "blank lines and spaces": b"   \n\n  \t \n",
    "many short lines": b"x\n" * 200_000,
    "long line then short": b"y" * 1_000_000 + b" z\nq\n",
}


def _strip_c(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return re.sub(r'"(?:\\.|[^"\\])*"', '""', text)


def wcx(ctx: VerifyContext) -> VerifierResult:
    with sandbox.scratch_copy(ctx.workdir) as work:
        src = work / "wcx.c"
        if not src.is_file():
            return VerifierResult("python", False, "wcx.c is missing")
        bad = sorted(set(FORBIDDEN_CALLS.findall(_strip_c(src.read_text(errors="replace")))))
        if bad:
            return VerifierResult("python", False, f"forbidden dynamic-memory calls: {bad}")
        r = sandbox.run(["gcc", *CFLAGS, "-o", "wcx", "wcx.c"], work, 120)
        if r.returncode != 0:
            return VerifierResult("python", False, "compile failed: " + (r.stderr.strip()[:300] or "no output"))
        failures = []
        for name, data in WCX_CASES.items():
            (work / "case.bin").write_bytes(data)
            r = sandbox.run(["bash", "-c", "./wcx < case.bin"], work, 30)
            want = " ".join(map(str, wcx_reference(data))) + "\n"
            if r.timed_out or r.returncode != 0 or r.stdout != want:
                got = "timeout" if r.timed_out else f"exit {r.returncode}, {r.stdout!r}"
                failures.append(f"{name}: expected {want!r}, got {got}")
        r = sandbox.run(["bash", "-c", f"head -c {BIG} /dev/zero | tr '\\0' a | ./wcx"], work, 90)
        want = f"0 1 {BIG} {BIG}\n"
        if r.timed_out or r.returncode != 0 or r.stdout != want:
            failures.append(f"64 MiB single line: expected {want!r}, got {'timeout' if r.timed_out else repr(r.stdout[:60])} "
                            f"(exit {r.returncode})")
    return VerifierResult("python", not failures, "; ".join(failures[:4]) + (f" (+{len(failures) - 4} more)" if len(failures) > 4 else "") or "ok")


# ---- harbor_answers -----------------------------------------------------------------------------------------------

TERMINAL_NAMES = ["Alder Point", "Brightwater", "Cormorant Quay", "Dunmore", "East Spit", "Fennick", "Garrow Bay", "Holloway",
                  "Ironbridge"]
ITEM_RE = re.compile(r"(?ms)^[ \t>*_-]*(\d)\s*[.)]\s*(.*?)(?=^[ \t>*_-]*\d\s*[.)]|\Z)")


def numbered_items(text: str) -> dict[int, str]:
    """{n: text of item n} for answers written as a numbered list (1. ... 2. ... or 1) ...)."""
    return {int(m.group(1)): m.group(2) for m in ITEM_RE.finditer(text)}


def _date(day: int, month: str, year: int, iso: str) -> re.Pattern:
    return re.compile(rf"(?i)\b{day}(?:st|nd|rd|th)?\s+{month}[a-z]*\.?,?\s+{year}\b|\b{month}[a-z]*\.?\s+{day}(?:st|nd|rd|th)?,?\s+{year}\b|{iso}")


def harbor_answers(ctx: VerifyContext) -> VerifierResult:
    facts = json.loads((ctx.verify_root / ctx.spec.id / "facts.json").read_text())
    items = numbered_items(ctx.final_answer)
    fails = []
    for n in range(1, 7):
        if n not in items:
            fails.append(f"item {n} missing")
    total = facts["total_throughput_ktueu"]
    if 1 in items and not re.search(rf"\b{str(total)[0]}[,.\s]?{str(total)[1:]}\b", items[1]):
        fails.append(f"1: total {total} not stated")
    if 2 in items:
        rate = f"{facts['highest_rate']:.2f}".replace(".", r"[.,]")
        if facts["highest_rate_terminal"] not in items[2] or not re.search(rate[:-1] + r"\d?", items[2]):
            fails.append(f"2: expected {facts['highest_rate_terminal']} with rate {facts['highest_rate']}")
    if 3 in items and not re.search(rf"{facts['downtime_change_pct']}".replace(".", r"[.,]"), items[3]):
        fails.append(f"3: expected a change of {facts['downtime_change_pct']} %")
    if 4 in items:
        named = {n for n in TERMINAL_NAMES if n.lower() in items[4].lower()}
        if named != set(facts["green_berth"]):
            fails.append(f"4: named {sorted(named)}, expected {sorted(facts['green_berth'])}")
    if 5 in items and not (re.search(r"(?i)\bboard\b", items[5]) and _date(21, "nov", 2025, "2025-11-21").search(items[5])):
        fails.append("5: expected the Board of Directors on 21 November 2025")
    if 6 in items and not _date(1, "apr", 2026, "2026-04-01").search(items[6]):
        fails.append("6: expected 1 April 2026")
    return VerifierResult("python", not fails, "; ".join(fails) or "ok")


# ---- tz_slot ------------------------------------------------------------------------------------------------------

TZ_ATTENDEES = {"ana@example.com": "14:30", "ben@example.com": "09:30", "chen@example.com": "15:30"}  # local start of the slot
TZ_START, TZ_END = datetime(2026, 9, 16, 13, 30), datetime(2026, 9, 16, 14, 15)  # UTC, the only correct answer
TZ_DATE_RE = re.compile(r"(?i)2026-09-16|\b16(?:th)?\s+sep(?:t|tember)?\b|\bsep(?:t|tember)?\.?\s+16(?:th)?\b|\b16/09/2026\b|\bWednesday\b")


def _utc(ts: str) -> datetime | None:
    """ISO 8601 timestamp (naive means UTC, as the task says) -> naive UTC datetime, or None if unparseable."""
    try:
        d = datetime.fromisoformat(ts.strip().replace("Z", "+00:00"))
    except ValueError:
        return None
    return d.astimezone(UTC).replace(tzinfo=None) if d.tzinfo else d


def _store(path: Path) -> list:
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return []
    return data if isinstance(data, list) else []


def tz_slot(ctx: VerifyContext) -> VerifierResult:
    state = ctx.state_dir or ctx.workdir
    events, emails = _store(state / "calendar_store.json"), _store(state / "email_store.json")
    fails = []
    if len(events) != 1:
        fails.append(f"{len(events)} events created, expected 1")
    else:
        e = events[0]
        start, end = _utc(e.get("start", "")), _utc(e.get("end", ""))
        if (start, end) != (TZ_START, TZ_END):
            fails.append(f"event at {e.get('start')} - {e.get('end')}, expected {TZ_START:%Y-%m-%dT%H:%M} - {TZ_END:%H:%M} UTC")
        if {a.strip().lower() for a in e.get("attendees", [])} != set(TZ_ATTENDEES):
            fails.append(f"attendees {e.get('attendees')}")
    if len(emails) != len(TZ_ATTENDEES):
        fails.append(f"{len(emails)} emails sent, expected {len(TZ_ATTENDEES)}")
    seen = {}
    for m in emails:
        seen.setdefault(m.get("to", "").strip().lower(), []).append(m)
    for who, local in TZ_ATTENDEES.items():
        mine = seen.get(who, [])
        if len(mine) != 1:
            fails.append(f"{len(mine)} emails to {who}, expected 1")
            continue
        body = mine[0].get("body", "")
        if not re.search(rf"(?<!\d){local}(?!\d)", body):
            fails.append(f"email to {who} lacks the local start time {local}")
        if not TZ_DATE_RE.search(body):
            fails.append(f"email to {who} lacks the date")
    return VerifierResult("python", not fails, "; ".join(fails) or "ok")
