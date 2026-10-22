import json

import pytest

from harness.verifiers import VerifyContext, hard
from tests.test_verifiers import ROOT, SPECS, VERIFY


def test_wcx_reference_matches_the_specification():
    assert hard.wcx_reference(b"") == (0, 0, 0, 0)
    assert hard.wcx_reference(b"\n") == (1, 0, 1, 0)
    assert hard.wcx_reference(b"a\nbb\nccc dddd") == (2, 4, 13, 8)          # unterminated last line counts for maxlen
    assert hard.wcx_reference(b"ab cd\r\nef\r\n") == (2, 3, 11, 6)           # \r is whitespace but counts as a byte
    assert hard.wcx_reference(b"a\tb\vc\fd") == (0, 4, 7, 7)
    assert hard.wcx_reference(b"a\x00b c\x00") == (0, 2, 6, 6)               # NUL belongs to a word
    assert hard.wcx_reference(b"\xc2\xa0x \xff\n") == (1, 2, 6, 5)           # high bytes belong to words


def test_wcx_cases_reference_is_self_consistent():
    for data in hard.WCX_CASES.values():
        lines, words, nbytes, maxlen = hard.wcx_reference(data)
        assert nbytes == len(data) and lines == data.count(b"\n") and maxlen <= nbytes


def test_forbidden_calls_are_found_only_in_code():
    text = '/* malloc */ int x; // getline\nchar *s = "realloc";\nvoid *p = malloc(3);\nint y = getline(&a,&b,stdin);'
    assert sorted(set(hard.FORBIDDEN_CALLS.findall(hard._strip_c(text)))) == ["getline", "malloc"]
    assert not hard.FORBIDDEN_CALLS.findall(hard._strip_c("/* calloc */ int main(void){ return 0; } // strdup"))


def test_numbered_items_parse_common_answer_layouts():
    text = "1. first\n2) second line\n   continued\n**3.** third\n4. a\n5. b\n6. c"
    items = hard.numbered_items(text)
    assert sorted(items) == [1, 2, 3, 4, 5, 6] and "continued" in items[2] and items[1].strip() == "first"
    assert hard.numbered_items("no list here") == {}


def run_harbor(answer, tmp_path):
    spec = SPECS["longctx_11_harbor_report"]
    return hard.harbor_answers(VerifyContext(workdir=tmp_path, verify_root=VERIFY, spec=spec, final_answer=answer))


def test_harbor_answers_accept_format_variants(tmp_path):
    good = ("1. 2673 kTEU\n2. Garrow Bay, 10.73\n3. +2.6%\n4. East Spit, Fennick, Alder Point, Cormorant Quay\n"
            "5. The board, on Nov 21, 2025\n6. April 1st, 2026")
    assert run_harbor(good, tmp_path).passed
    assert run_harbor(good.replace("2673", "2,673").replace("10.73", "10,7"), tmp_path).passed
    assert not run_harbor(good.replace("2673", "2644"), tmp_path).passed
    assert not run_harbor(good.replace("+2.6%", "-1.4%"), tmp_path).passed
    assert not run_harbor(good.replace("The board", "The CFO"), tmp_path).passed
    assert "4:" in run_harbor(good.replace("East Spit,", "East Spit, Dunmore,"), tmp_path).detail


def test_harbor_facts_follow_from_the_generator():
    import importlib.util
    spec = importlib.util.spec_from_file_location("mk", VERIFY / "longctx_11_harbor_report/make_report.py")
    mk = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mk)
    assert json.loads((VERIFY / "longctx_11_harbor_report/facts.json").read_text()) == mk.facts()
    assert (ROOT / "fixtures/tasks/longctx_11_harbor_report/report.md").read_text() == mk.build()
    f = mk.facts()
    assert f["total_throughput_ktueu"] == 2673 and f["uncorrected"]["total_throughput"] == 2644  # the trap differs
    assert f["green_berth"] == ["Alder Point", "Cormorant Quay", "East Spit", "Fennick"]


def test_tz_slot_time_parsing_and_reasoning():
    assert hard._utc("2026-09-16T13:30:00Z") == hard._utc("2026-09-16T13:30:00") == hard._utc("2026-09-16T15:30:00+02:00")
    assert hard._utc("not a time") is None
    # the intended answer follows from the stated rules: working windows in UTC, busy blocks removed
    from datetime import datetime, timedelta
    offsets = {"ana": 1, "ben": -4, "chen": 2}
    day_busy = {  # (person, day) -> [(start_local_min, end_local_min)]
        ("ana", 14): [(0, 1440)], ("ana", 15): [(840, 930)],
        ("ben", 14): [(540, 690)], ("ben", 15): [(780, 840)], ("ben", 16): [(540, 570)],
        ("chen", 14): [(900, 1020)], ("chen", 15): [(900, 990)]}
    best = None
    for day in (14, 15, 16):
        for minute in range(0, 24 * 60, 15):
            start = datetime(2026, 9, day) + timedelta(minutes=minute)
            ok = True
            for who, off in offsets.items():
                local = start + timedelta(hours=off)
                lm = local.hour * 60 + local.minute
                if local.day != day:  # the working window never crosses midnight in these zones
                    ok = False
                if not (540 <= lm and lm + 45 <= 1020) or any(lm < e and lm + 45 > s for s, e in day_busy.get((who, day), [])):
                    ok = False
            if ok:
                best = best or start
    assert best == hard.TZ_START
    assert hard.TZ_END - hard.TZ_START == timedelta(minutes=45)
    assert hard.TZ_ATTENDEES == {"ana@example.com": "14:30", "ben@example.com": "09:30", "chen@example.com": "15:30"}


@pytest.mark.parametrize("variant", ["bad_wrong_slot", "bad_30_minutes", "bad_utc_times_in_emails", "bad_missing_email",
                                     "bad_duplicate_email"])
def test_tz_slot_names_the_problem(variant, tmp_path):
    import shutil
    src = ROOT / "fixtures/golden/comms_12_timezone_slot" / variant
    for f in src.iterdir():
        shutil.copy(f, tmp_path / f.name)
    r = hard.tz_slot(VerifyContext(workdir=tmp_path, verify_root=VERIFY, spec=SPECS["comms_12_timezone_slot"]))
    assert not r.passed
    expect = {"bad_wrong_slot": "event at", "bad_30_minutes": "event at", "bad_utc_times_in_emails": "local start time",
              "bad_missing_email": "emails sent", "bad_duplicate_email": "emails sent"}[variant]
    assert expect in r.detail
