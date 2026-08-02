#!/usr/bin/env python3
"""
comm_tools_mcp.py

MCP server exposing send_email and create_event as agent tools, built to
demonstrate the assignment's required communication-workflow behaviors:
authorization/confirmation before a consequential action, partial failure
handling, and duplicate-execution prevention.

Execution model (set via env var COMM_TOOLS_MODE, default "mock"):
  mock  - calls the bundled send_email.sh / create_event.sh shipped in this
          same directory. Both are sandboxed: they append to local JSON
          files (email_store.json, calendar_store.json) and never touch a
          real mail server or calendar service. Safe default.
  live  - calls "send_email.sh" / "create_event.sh" by bare name, resolved
          via the shell PATH — NOT the bundled sandboxed scripts. To
          actually go live, place functional versions of both scripts
          somewhere on PATH under those exact names. This is deliberate:
          going live requires two separate, explicit steps (setting the
          env var AND installing real scripts on PATH), so it can't happen
          by accident.

Confirmation flow:
  Both tools require confirm=false (default) on the first call, which
  returns a preview of the action WITHOUT performing it. The agent must
  call again with confirm=true to actually execute — this is the
  "confirmation before consequential action" requirement.

Duplicate-execution handling:
  Each confirmed action is hashed into an idempotency key. If that key
  was already successfully executed, a repeat confirm=true call is a
  no-op that reports the prior result instead of re-sending/re-creating.

Simulated partial failure (for testing the agent's retry behavior):
  Set env var COMM_TOOLS_FORCE_FAIL_ONCE=true. The first confirmed
  send_email call for a NEW idempotency key will fail with a simulated
  transient error; a retry of the same call succeeds and is recorded
  normally (further retries after that are caught as duplicates).

Install:
    pip install "mcp[cli]==1.28.1" --ignore-installed PyJWT
Run (stdio transport, matches the pattern already used for searxng):
    python3 comm_tools_mcp.py
"""

import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from mcp.server.fastmcp import FastMCP

BASE = Path(__file__).parent
MODE = os.environ.get("COMM_TOOLS_MODE", "mock")
FORCE_FAIL_ONCE = os.environ.get("COMM_TOOLS_FORCE_FAIL_ONCE", "false").lower() == "true"

DEDUP_PATH = BASE / "executed_actions.json"
FAILED_ONCE_PATH = BASE / ".failed_once_keys.json"

mcp = FastMCP("comm-tools")


def _load_json(path: Path, default):
    if not path.exists():
        return default
    try:
        with open(path) as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError):
        return default


def _save_json(path: Path, data):
    with open(path, "w") as f:
        json.dump(data, f, indent=2)


def _idempotency_key(*parts: str) -> str:
    return hashlib.sha256("||".join(parts).encode()).hexdigest()[:16]


def _already_executed(key: str):
    executed = _load_json(DEDUP_PATH, {})
    return executed.get(key)


def _record_executed(key: str, result: dict):
    executed = _load_json(DEDUP_PATH, {})
    executed[key] = result
    _save_json(DEDUP_PATH, executed)


def _should_fail_once(key: str) -> bool:
    if not FORCE_FAIL_ONCE:
        return False
    failed_keys = _load_json(FAILED_ONCE_PATH, [])
    if key in failed_keys:
        return False  # already failed once for this key, let it succeed now
    failed_keys.append(key)
    _save_json(FAILED_ONCE_PATH, failed_keys)
    return True


def _resolve_script(name: str) -> str:
    """mock: always the bundled sandboxed script in this directory.
    live: bare name, resolved via PATH — requires the user to have
    installed a real script there. Raises if not found in live mode."""
    if MODE == "mock":
        return str(BASE / name)
    found = shutil.which(name)
    if not found:
        raise FileNotFoundError(
            f"COMM_TOOLS_MODE=live but no '{name}' found on PATH. "
            f"Install a functional version of {name} on PATH to go live."
        )
    return found


@mcp.tool()
def send_email(recipient: str, subject: str = "", body: str = "", confirm: bool = False) -> str:
    """Send an email. This is a consequential action. subject is optional.

    On the first call (confirm=false, the default), this returns a preview
    of the email WITHOUT sending it. You must call this tool again with the
    exact same arguments and confirm=true to actually send it. Only set
    confirm=true after the user has approved the preview, or if the user's
    instruction already explicitly authorized sending without further
    confirmation.
    """
    key = _idempotency_key("email", recipient, subject, body)

    if not confirm:
        return (
            "PREVIEW (not sent). To actually send this email, call "
            "send_email again with confirm=true and identical arguments.\n"
            f"To: {recipient}\nSubject: {subject or '(none)'}\nBody: {body}"
        )

    prior = _already_executed(key)
    if prior:
        return f"Skipped: an identical email was already sent (idempotency key {key}). Not sending again. Prior result: {prior['result']}"

    if _should_fail_once(key):
        return (
            f"ERROR: transient failure sending email to {recipient} "
            f"(simulated SMTP timeout). The email was NOT sent. You may retry."
        )

    try:
        script = _resolve_script("send_email.sh")
    except FileNotFoundError as e:
        return f"ERROR: {e}"

    cmd = [script]
    if subject:
        cmd += ["-s", subject]
    cmd += [recipient, body]

    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        return f"ERROR: send_email.sh failed: {proc.stderr.strip()}"
    result = proc.stdout.strip()

    _record_executed(key, {"result": result})
    return f"Email sent to {recipient}. {result}"


@mcp.tool()
def create_event(title: str, start: str, end: str, attendees: str = "", confirm: bool = False) -> str:
    """Create a calendar event. This is a consequential action.

    start and end must be ISO 8601 timestamps (e.g. 2026-08-05T10:00:00).
    attendees is a comma-separated list of email addresses, or empty.

    On the first call (confirm=false, the default), this returns a preview
    of the event WITHOUT creating it. You must call this tool again with
    the exact same arguments and confirm=true to actually create it.
    """
    normalized_attendees = ",".join(sorted(a.strip().lower() for a in attendees.split(",") if a.strip()))
    key = _idempotency_key("event", title.strip(), start.strip(), end.strip(), normalized_attendees)

    if not confirm:
        return (
            "PREVIEW (not created). To actually create this event, call "
            "create_event again with confirm=true and identical arguments.\n"
            f"Title: {title}\nStart: {start}\nEnd: {end}\nAttendees: {attendees or '(none)'}"
        )

    prior = _already_executed(key)
    if prior:
        return f"Skipped: an identical event was already created (idempotency key {key}). Prior result: {prior['result']}"

    try:
        script = _resolve_script("create_event.sh")
    except FileNotFoundError as e:
        return f"ERROR: {e}"

    proc = subprocess.run([script, title, start, end, attendees], capture_output=True, text=True)
    if proc.returncode != 0:
        return f"ERROR: create_event.sh failed: {proc.stderr.strip()}"
    result = proc.stdout.strip()

    _record_executed(key, {"result": result})
    return f"Event created: {title}. {result}"


if __name__ == "__main__":
    print(f"comm-tools MCP server starting (mode={MODE}, force_fail_once={FORCE_FAIL_ONCE})", file=sys.stderr)
    mcp.run()
