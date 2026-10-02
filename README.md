# Lab 2: AI Code Review and Memory-Safety Validation

Tom Wilkinson - CYB 621 - October 2, 2026

## GitHub repository

[Pygmy1987 / CYB 621 Lab 2](https://github.com/Pygmy1987/cybr621-lab2-CODESPACES-AI-Code-Review-and-Memory-Safety-Validation-)

## Package contents

This package contains the final report, all ten reflection answers, original and corrected source, reproducible commands, runtime evidence, static-analysis results, and six selected screenshots placed with the related reflection answers.

## Results

- Original Assistant 1 accepted an oversized username and newline injection. Its observed log mode was 666.
- Original Assistant 2 rejected those inputs and refused that broadly accessible log. It created a fresh mode-600 log, but rejected the combined maximum valid input because its record buffer was one byte short.
- Corrected Assistant 1 validates input and creates new logs with mode 0600. Corrected Assistant 2 allocates 4164 bytes for a maximum 4163-byte record plus its terminating NUL.
- Both corrected programs compiled without warnings in the recorded warning-enabled builds.
- Final ordinary and ASan/UBSan executables passed all 20 cases. Rejected input left the log unchanged; all four maximum records measured 4163 bytes. No sanitizer diagnostic appears in the final log.
- Semgrep auto: 52 applicable rules, 2 files, 0 findings before and after. Custom scan: 2 rules, 2 files, 0 findings before and after.
- CodeQL: 1 original cpp/world-writable-file-creation finding at original assistant1.c:12; 0 findings after remediation.

Zero static findings and passing runtime tests do not establish production readiness. See the report for manual findings and release conditions.

## Files

- `assistant1.c`, `assistant2.c`: corrected sources.
- `evidence/originals/`: preserved AI-generated source, before remediation.
- `prompts.md`: exact generation prompts, displayed model information, and prediction.
- `semgrep.yml`: two custom rules used in this lab.
- `semgrep-results.json`, `codeql-results.sarif`: final root scan outputs.
- `evidence/`: original and corrected build logs, runtime observations, tool versions, before/after scan outputs.
- `evidence/screenshots/`: six selected screenshots embedded beside the relevant answers; `screenshot-index.csv` maps their captions, original filenames, hashes, and PDF pages.
- `scripts/final-regression.sh`: saved version of the final test harness used in the Codespace.
- `Lab2_Report.pdf`, `Lab2_Report.md`: formatted and editable final report.
- `userlog.txt`: accumulated synthetic lab data; includes intentionally forged baseline entries and long test records. It is not a production audit log.

Compiled executables, object files, the generated CodeQL database, and the temporary review archive are excluded by `.gitignore`.

## Environment

Tests were performed in a Linux GitHub Codespace. Recorded versions: GCC 13.3.0, Git 2.55.0, Semgrep 1.179.0, CodeQL 2.27.1, and codeql/cpp-queries 1.9.0. Version evidence is under `evidence/`.

The archived `umask.txt` says 0022, while the original normal-test stat output says 666. These do not establish the same file-creation conditions. Fresh 0666 creation under 0022 would normally yield 0644. The report preserves this uncertainty. Explicit 0600 creation and a fresh-file test demonstrate the corrected behavior without assuming how the earlier file obtained its mode.

## Reproduce builds and runtime tests

Run from the repository root in Linux with GCC available. Do not run these POSIX-specific programs as native Windows executables. The commands below place new observations under evidence/recheck so the supplied evidence is retained.

```bash
mkdir -p evidence/recheck
for n in 1 2; do
    gcc -Wall -Wextra -Wpedantic "assistant$n.c" -o "assistant$n" > "evidence/recheck/assistant$n-build.txt" 2>&1
    result=$?
    echo "Compiler exit code: $result" >> "evidence/recheck/assistant$n-build.txt"
    if [ "$result" -ne 0 ]; then cat "evidence/recheck/assistant$n-build.txt"; break; fi
    gcc -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined -fno-omit-frame-pointer "assistant$n.c" -o "assistant${n}_san" > "evidence/recheck/assistant$n-sanitizer-build.txt" 2>&1
    result=$?
    echo "Compiler exit code: $result" >> "evidence/recheck/assistant$n-sanitizer-build.txt"
    if [ "$result" -ne 0 ]; then cat "evidence/recheck/assistant$n-sanitizer-build.txt"; break; fi
done
```

Check that all four builds succeeded before running:

```bash
bash scripts/final-regression.sh > evidence/recheck/final-runtime-regression.txt 2>&1
cat evidence/recheck/final-runtime-regression.txt
```

Expected: 20 PASS lines, no FAIL lines or sanitizer diagnostics, and four `Maximum record bytes: 4163` lines. Normal and maximum-length tests expect exit0; missing arguments expect exit1 for Assistant1 and exit2 for Assistant2; long username and injection expect exit2. The harness checks unchanged log contents for rejected cases. A PASS line is not itself proof that a sanitizer emitted no warning; inspect the complete output. Maximum record size is printed for separate review. The harness starts with a mode600 file; fresh creation was measured separately.

To recheck fresh creation without changing the repository log:

```bash
(
    program="$PWD/assistant1"
    test_dir=$(mktemp -d) || exit 1
    cd "$test_dir" || exit 1
    "$program" student1 "Fresh file permission test"
    echo "Program exit code: $?"
    cat userlog.txt
    stat -c '%A %a %n' userlog.txt
) > evidence/recheck/fresh-permissions.txt 2>&1
cat evidence/recheck/fresh-permissions.txt
```

The recorded fresh test returned0 and mode600. Existing files retain their permissions with Assistant1; this is a limitation, not a promised remediation of existing logs.

## Reproduce static scans

With Semgrep installed in the Codespace:

```bash
semgrep scan --config=auto assistant1.c assistant2.c --json --output=evidence/recheck/semgrep.json > evidence/recheck/semgrep.txt 2>&1
echo "Scan exit code: $?" >> evidence/recheck/semgrep.txt
semgrep scan --config=semgrep.yml assistant1.c assistant2.c > evidence/recheck/semgrep-custom.txt 2>&1
echo "Scan exit code: $?" >> evidence/recheck/semgrep-custom.txt
```

`--config=auto` uses registry rules that can change. The archived outputs record the results at lab time; pin the resolved rule configuration for deterministic CI rather than assuming future auto runs are identical. The custom YAML is included exactly as used.

With the GitHub CLI CodeQL extension installed, rebuild the generated database (replaces its existing contents):

```bash
gh codeql database create ./codeql-db --language=cpp --command='gcc -Wall -Wextra -Wpedantic -c assistant1.c assistant2.c' --overwrite > evidence/recheck/codeql-build.txt 2>&1
echo "Database build exit code: $?" >> evidence/recheck/codeql-build.txt
```

Only after the database build succeeds:

```bash
gh codeql database analyze ./codeql-db 'codeql/cpp-queries:codeql-suites/cpp-security-and-quality.qls' --format=sarif-latest --output=evidence/recheck/codeql.sarif --download > evidence/recheck/codeql-analysis.txt 2>&1
echo "Analysis exit code: $?" >> evidence/recheck/codeql-analysis.txt
jq '[.runs[].results[]?] | length' evidence/recheck/codeql.sarif
```

The supplied run used query pack1.9.0. The command can resolve a different pack on a later date; record or pin its version for reproducibility. The recorded build traced the two corrected root C files. The two original backups are not part of that final compilation; the coverage message reports2/4 project C/C++ files.

## Evidence interpretation and recommendation

Keep baseline and corrected results distinct. The original file-creation warning matched an observed broadly writable file. The final fresh600 test and zero CodeQL findings support the targeted fix. Neither source calls localtime, so the instructor's example localtime finding did not apply. No memory-corruption finding was observed; the boundary bug was a checked formatting rejection.

Start from corrected Assistant2 for further development because it has stronger descriptor checks, nofollow, and cooperating-writer serialization. Before a production release, address authenticated identity, trusted path handling, special-file blocking, concurrency and disk-exhaustion tests, reliable error reporting, and a defined persistence policy. Human review remains required.
