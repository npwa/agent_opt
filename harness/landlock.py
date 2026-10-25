"""
harness/landlock.py

Filesystem confinement of the agent with Linux Landlock (no root, no user namespace needed).

Why: OpenCode runs with every permission allowed and with the full rights of the user. A weaker model
(qwen3:8b in the first screen) searched the whole filesystem for the file it had to change and edited the
repository's own task fixtures and golden files instead of its working directory. A trial must not be able
to read or change anything outside its own directories: not the repo (hidden verifier assets live in
fixtures/verify), not the home directory (keys, tokens), not other trials.

How: ``python -m harness.landlock --rw DIR ... [--ro PATH ...] -- command args`` restricts the current
process (irreversibly, inherited by every child) and then execs the command. Everything that is not
granted is denied. Read-only system locations and the tools OpenCode needs (its binary, node for the MCP
servers) are granted by default (see DEFAULT_RO); each ``--rw`` path is readable, writable and creatable.
Network access is not restricted (Ollama, the proxy and SearXNG are on localhost).

Landlock grants are additive: a path below a granted directory cannot be excluded again, so the grants
are kept as narrow as the tools allow (the trial directory instead of /tmp).
"""

import argparse
import ctypes
import os
import struct
import sys
from pathlib import Path

SYS_CREATE, SYS_ADD, SYS_RESTRICT = 444, 445, 446  # x86_64
RULE_PATH_BENEATH = 1
PR_SET_NO_NEW_PRIVS = 38

# access rights of Landlock ABI 1-3 (filesystem)
EXECUTE, WRITE_FILE, READ_FILE, READ_DIR = 1 << 0, 1 << 1, 1 << 2, 1 << 3
REMOVE_DIR, REMOVE_FILE, MAKE_CHAR, MAKE_DIR, MAKE_REG = 1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8
MAKE_SOCK, MAKE_FIFO, MAKE_BLOCK, MAKE_SYM, REFER, TRUNCATE = 1 << 9, 1 << 10, 1 << 11, 1 << 12, 1 << 13, 1 << 14
ALL_FS = (1 << 15) - 1
READ_ONLY = EXECUTE | READ_FILE | READ_DIR

HOME = Path.home()
DEFAULT_RO = [Path(p) for p in ("/usr", "/lib", "/lib32", "/lib64", "/bin", "/sbin", "/etc", "/opt", "/proc", "/sys", "/var/lib",
                                "/run/systemd/resolve")] + [  # resolve: /etc/resolv.conf points there
    HOME / ".opencode", HOME / ".nvm", HOME / ".local/lib"]  # .local/lib: the user's Python packages (the MCP library)
DEFAULT_RW = [Path("/dev"), HOME / ".npm"]  # device nodes (/dev/null), npx's package cache


class LandlockError(RuntimeError):
    pass


def abi_version() -> int:
    """Highest Landlock ABI the kernel supports (0 = not available)."""
    libc = ctypes.CDLL(None, use_errno=True)
    v = libc.syscall(SYS_CREATE, None, 0, 1)  # LANDLOCK_CREATE_RULESET_VERSION
    return max(v, 0)


def supported() -> bool:
    return sys.platform == "linux" and abi_version() >= 1


def restrict(rw: list[Path], ro: list[Path] | None = None) -> None:
    """Confine this process and its future children: ``ro`` readable/executable, ``rw`` everything, the rest denied."""
    libc = ctypes.CDLL(None, use_errno=True)
    abi = abi_version()
    if abi < 1:
        raise LandlockError("Landlock is not available on this kernel")
    handled = ALL_FS if abi >= 3 else (ALL_FS & ~TRUNCATE if abi >= 2 else ALL_FS & ~(TRUNCATE | REFER))
    attr = struct.pack("QQ", handled, 0) if abi >= 4 else struct.pack("Q", handled)
    fd = libc.syscall(SYS_CREATE, attr, len(attr), 0)
    if fd < 0:
        raise LandlockError(f"landlock_create_ruleset failed (errno {ctypes.get_errno()})")
    try:
        for paths, access in ((ro or [], READ_ONLY), (rw, handled)):  # access == READ_ONLY identifies the optional read-only list
            for p in paths:
                p = Path(p)
                if not p.exists():
                    if access == READ_ONLY:
                        continue  # an optional system path that this machine does not have
                    raise LandlockError(f"writable path does not exist: {p} (a missing grant would silently deny the agent)")
                pfd = os.open(p, os.O_PATH | os.O_CLOEXEC)
                try:
                    # a single file cannot get directory rights: keep file-applicable ones only
                    acc = access & (EXECUTE | WRITE_FILE | READ_FILE | TRUNCATE) if not p.is_dir() else access
                    rule = struct.pack("=Qi", acc & handled, pfd)  # packed: u64 allowed_access, s32 parent_fd
                    if libc.syscall(SYS_ADD, fd, RULE_PATH_BENEATH, rule, 0) != 0:
                        raise LandlockError(f"landlock_add_rule failed for {p} (errno {ctypes.get_errno()})")
                finally:
                    os.close(pfd)
        if libc.prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0:
            raise LandlockError("prctl(NO_NEW_PRIVS) failed")
        if libc.syscall(SYS_RESTRICT, fd, 0) != 0:
            raise LandlockError(f"landlock_restrict_self failed (errno {ctypes.get_errno()})")
    finally:
        os.close(fd)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Run a command confined with Landlock")
    ap.add_argument("--rw", type=Path, action="append", default=[], help="directory the command may read and change (repeatable)")
    ap.add_argument("--ro", type=Path, action="append", default=[], help="extra read-only path (repeatable)")
    ap.add_argument("--no-defaults", action="store_true", help="do not add the default read-only system paths")
    ap.add_argument("cmd", nargs=argparse.REMAINDER)
    a = ap.parse_args(argv)
    cmd = a.cmd[1:] if a.cmd[:1] == ["--"] else a.cmd
    if not cmd:
        ap.error("no command given")
    ro = a.ro + ([] if a.no_defaults else DEFAULT_RO)
    rw = a.rw + ([] if a.no_defaults else [p for p in DEFAULT_RW if p.exists()])  # ~/.npm is absent on a fresh machine
    try:
        restrict(rw, ro)
    except LandlockError as e:
        print(f"landlock: {e}", file=sys.stderr)
        return 126
    os.execvp(cmd[0], cmd)


if __name__ == "__main__":
    raise SystemExit(main())
