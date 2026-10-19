"""Programmatic rubric checks on text: length limits, required facts, forbidden claims."""

import re

from harness.verifiers import VerifierResult, VerifyContext

FLAGS = re.IGNORECASE | re.DOTALL


def verify(params: dict, ctx: VerifyContext) -> VerifierResult:
    target = params.get("target", "final_answer")
    if target == "final_answer":
        text = ctx.final_answer
    else:
        path = (ctx.workdir / target).resolve()
        if not path.is_relative_to(ctx.workdir.resolve()) or not path.is_file():
            return VerifierResult("rubric", False, f"target file '{target}' missing")
        text = path.read_text(errors="replace")

    failures = []
    words = len(text.split())
    if "min_words" in params and words < params["min_words"]:
        failures.append(f"{words} words < min {params['min_words']}")
    if "max_words" in params and words > params["max_words"]:
        failures.append(f"{words} words > max {params['max_words']}")
    for item in params.get("required", []):
        if not any(re.search(p, text, FLAGS) for p in item["any_of"]):
            failures.append(f"missing '{item['name']}'")
    for item in params.get("forbidden", []):
        if re.search(item["regex"], text, FLAGS):
            failures.append(f"forbidden '{item['name']}'")
    return VerifierResult("rubric", not failures, "; ".join(failures) or f"{words} words, all checks ok")
