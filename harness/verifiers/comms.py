"""Verifier for the comms workflow: inspect side-effect stores written by the mock tools."""

import json
from pathlib import Path

from harness.verifiers import VerifierResult, VerifyContext


def _load(path: Path) -> list | None:
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return None
    return data if isinstance(data, list) else None


def _email_key(e: dict) -> tuple:
    return (e.get("to", "").strip().lower(), e.get("subject", "").strip(), e.get("body", "").strip())


def _event_key(e: dict) -> tuple:
    attendees = tuple(sorted(a.strip().lower() for a in e.get("attendees", [])))
    return (e.get("title", "").strip(), e.get("start", "").strip(), e.get("end", "").strip(), attendees)


def no_duplicate_actions(ctx: VerifyContext, expected_emails: int = 1,
                         expected_events: int = 1) -> VerifierResult:
    """Exactly the expected number of emails/events exist and none is a duplicate.

    The stores (email_store.json / calendar_store.json) are the ground truth of what
    really happened; the idempotency file alone could hide a duplicate that was suppressed
    after the model already tried it, so only the stores are used.
    """
    state = ctx.state_dir or ctx.workdir
    emails = _load(state / "email_store.json")
    events = _load(state / "calendar_store.json")
    if emails is None:
        emails = []
    if events is None:
        events = []
    failures = []
    if len(emails) != expected_emails:
        failures.append(f"{len(emails)} emails sent, expected {expected_emails}")
    if len(events) != expected_events:
        failures.append(f"{len(events)} events created, expected {expected_events}")
    if len({_email_key(e) for e in emails}) != len(emails):
        failures.append("duplicate email")
    if len({_event_key(e) for e in events}) != len(events):
        failures.append("duplicate event")
    return VerifierResult("python", not failures, "; ".join(failures) or "ok")
