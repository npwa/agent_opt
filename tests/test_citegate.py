from harness import citegate as cg

EVIDENCE = "Soon CS, Brass M, Heinze HJ, Haynes JD. Nature Neuroscience 2008."
ANSWER = "Soon et al. (2008) decoded choices. Nunnally et al. (2019) disagreed with them."


def test_arms_and_scores():
    rows = cg.evaluate([{"trial": "t", "answer": ANSWER, "evidence": EVIDENCE}],
                       lambda ans, flagged: ans.replace("Nunnally et al. (2019)", "Others") if flagged == ["Nunnally et al. (2019)"] else ans)
    by = {r["arm"]: r for r in rows}
    assert set(by) == {"none", "mark", "drop", "rewrite"}
    assert (by["none"]["citations"], by["none"]["flagged"]) == (2, 1)
    assert by["mark"]["flagged"] == 1 and by["mark"]["supported_kept"] == 1
    assert (by["drop"]["citations"], by["drop"]["flagged"], by["drop"]["supported_kept"]) == (1, 0, 1)
    assert by["rewrite"]["flagged"] == 0 and by["rewrite"]["supported_kept"] == 1


def test_no_rewrite_arm_without_a_model_and_report():
    rows = cg.evaluate([{"trial": "t", "answer": ANSWER, "evidence": EVIDENCE}], None)
    assert {r["arm"] for r in rows} == {"none", "mark", "drop"}
    assert "| drop |" in cg.render(rows)


def test_rewrite_is_skipped_when_nothing_is_flagged():
    calls = []
    cg.evaluate([{"trial": "t", "answer": "Soon et al. (2008) is fine.", "evidence": EVIDENCE}], lambda a, f: calls.append(f) or a)
    assert calls == []
