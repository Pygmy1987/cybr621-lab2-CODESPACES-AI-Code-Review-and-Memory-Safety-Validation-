# Assignment completion map

The files below cover the lab requirements. Repository upload, commit/push, and course submission are separate actions.

## Generate and preserve

Both exact prompts and the prediction are in prompts.md. Original sources are preserved in evidence/originals/.

## Build and challenge

Warning builds, normal input, missing arguments, long usernames, and newline injection have saved results for both original implementations.

## Memory-safety checks

Original and corrected ASan/UBSan builds and runs are saved. The maximum-length defect and its correction have separate evidence.

## Manual review

The findings and reflection answers explain validation, bounds, log integrity, permissions, error handling, cleanup, and limits of the tests.

## Static analysis

Semgrep auto/custom text and JSON results, CodeQL traced build logs, SARIF, version records, and before/after findings are included.

## Remediate and revalidate

Corrected source is at the project root. final-runtime-regression.txt records 20 passing cases and four 4163-byte maximum records.

## Reflect and recommend

All ten answers are approximately 100 words. The recommendation selects Assistant 2 as a starting point and identifies work required before production.

## Reproduce and inspect

README.md includes commands. scripts/final-regression.sh preserves the final test harness. Six selected screenshots appear with the related reflection answers; their original PNGs are included. All ten answers follow the handout order and are followed by the final recommendation.

## Repository and delivery

The report and README link to [the repository used for this lab](https://github.com/Pygmy1987/cybr621-lab2-CODESPACES-AI-Code-Review-and-Memory-Safety-Validation-). The final files still require upload, commit/push, and course submission. The handout calls for an assigned GitHub Classroom repository; the recorded repository was created separately, so its acceptability or transfer to the assigned repository must be confirmed before submission. Live repository contents were not verified during this final check.
