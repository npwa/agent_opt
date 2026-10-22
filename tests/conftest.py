from pathlib import Path

import pytest

FIXTURES = Path(__file__).resolve().parent.parent / "fixtures"


@pytest.fixture
def fixtures_dir() -> Path:
    return FIXTURES


@pytest.fixture(autouse=True)
def _no_landlock_in_unit_tests(monkeypatch):
    """The fake `opencode` executables of the tests start the repo's virtualenv Python, which a Landlock-confined
    process may not read; tests of the confinement itself switch it on explicitly (tests/test_landlock.py)."""
    from harness import runner
    monkeypatch.setattr(runner, "CONFINE", False)
