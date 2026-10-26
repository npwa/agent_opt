import json

from harness import citations as ci

# the nine citations of the audited free-will run (results/README.md): 3 confirmed (incl. one minor page error), 6 fabricated/unsupported
ANSWER = """Libet, Gleason, Wright (1983) found a readiness potential. Soon et al. (2008) decoded choices from fMRI.
Haynes & Rees (2011) extended this. Trevena & Miller (2010) disagreed. Schurger, Mojaverian, Cohen (2012) offered an alternative.
Wittmann, Schultze (2005) studied timing; Nunnally et al. (2019) too. The Clarke & Miller debate (2016-2024) continues.
Haynes & Schultze-Kraft (2016) commented."""
EVIDENCE = """Libet B, Gleason CA, Wright EW, Pearl DK. Brain 1983 106 623. Soon CS, Brass M, Heinze HJ, Haynes JD. Nature Neuroscience 2008.
Schurger A, Sitt JD, Dehaene S. PNAS 2012. Haynes JD, Schultze-Kraft M. PNAS 2016 comment. John-Dylan Haynes works on decoding."""


def verdicts(text, evidence):
    return {(c["first"], c["year"]): c["verdict"] for c in ci.verify(ci.extract(text), evidence) if c["kind"] == "author_year"}


def test_audited_run_is_reproduced():
    v = verdicts(ANSWER, EVIDENCE)
    assert v[("Libet", "1983")] == v[("Soon", "2008")] == v[("Haynes", "2016")] == "supported"
    assert v[("Haynes", "2011")] == "partial"  # surname known, year never near it
    assert v[("Schurger", "2012")] == "partial"  # invented co-authors
    for fabricated in (("Trevena", "2010"), ("Wittmann", "2005"), ("Nunnally", "2019"), ("Clarke", "2016")):
        assert v[fabricated] == "unsupported", fabricated
    assert sum(x != "supported" for x in v.values()) == 6


def test_urls_and_dois_need_to_occur_in_the_evidence():
    text = "See https://www.example.org/paper/1. Also https://nope.org/x and doi 10.1000/abc123."
    res = {c["raw"]: c["verdict"] for c in ci.verify(ci.extract(text), "page at http://example.org/paper/1 and doi 10.1000/abc123")}
    assert res["https://www.example.org/paper/1"] == "supported" and res["10.1000/abc123"] == "supported"
    assert res["https://nope.org/x"] == "unsupported"


def test_sentence_words_are_not_authors():
    assert not [c for c in ci.extract("In 2020 the market changed. During 2021 it grew. March 2022 was calm.") if c["kind"] == "author_year"]


def test_gate_marks_or_drops_flagged_citations_only():
    res = ci.verify(ci.extract("Soon et al. (2008) and Nunnally et al. (2019) agree."), EVIDENCE)
    assert ci.gate("Soon et al. (2008) and Nunnally et al. (2019) agree.", res) == "Soon et al. (2008) and Nunnally et al. (2019) [unverified] agree."
    assert ci.gate("Soon et al. (2008) and Nunnally et al. (2019) agree.", res, "drop") == "Soon et al. (2008) and  agree."


def test_read_trial_takes_text_after_the_last_tool_and_all_tool_output(tmp_path):
    def ev(**kw):
        return json.dumps(kw)
    lines = [ev(type="text", part={"text": "let me search"}),
             ev(type="tool_use", part={"tool": "search", "state": {"status": "completed", "output": "Soon 2008 page"}}),
             ev(type="text", part={"text": "Final: Soon et al. (2008)."})]
    p = tmp_path / "events.jsonl"
    p.write_text("\n".join(lines))
    answer, evidence = ci.read_trial(p)
    assert answer == "Final: Soon et al. (2008)." and evidence == "Soon 2008 page"


def test_run_analysis_and_render(tmp_path):
    d = tmp_path / "trials" / "cfg__task__00"
    d.mkdir(parents=True)
    (d / "record.json").write_text(json.dumps({"task": "task", "status": "pass"}))
    (d / "events.jsonl").write_text("\n".join([
        json.dumps({"type": "tool_use", "part": {"state": {"status": "completed", "output": "Soon 2008"}}}),
        json.dumps({"type": "text", "part": {"text": "Soon et al. (2008); Nunnally et al. (2019)."}})]))
    rows = ci.analyse_run(tmp_path)
    assert rows[0]["n"] == 2 and rows[0]["flagged"] == 1
    assert "1 flagged" in ci.render(rows) or "**1 flagged**" in ci.render(rows)


def test_decades_and_doi_links_are_not_extra_citations():
    got = [(c["kind"], c["raw"]) for c in ci.extract("Critiques (Mid-1990s) of the 1990s; see https://doi.org/10.1177/00197939 and Dube, Lipscomb, & Vijay (2011).")]
    assert [k for k, _ in got] == ["url", "author_year"] and got[1][1].startswith("Dube")
