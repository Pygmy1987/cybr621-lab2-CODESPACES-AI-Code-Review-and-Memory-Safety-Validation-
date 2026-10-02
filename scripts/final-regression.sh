#!/usr/bin/env bash
# Reproduces the final harness used in the Codespace; inspect the full output.
project_dir="$PWD"
run_case() {
    expected="$1"
    label="$2"
    shift 2
    cp userlog.txt before.txt || exit 1
    "$program" "$@"
    actual=$?
    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL: $label (expected $expected, got $actual)"
    elif [ "$expected" -ne 0 ] && ! cmp -s before.txt userlog.txt; then
        echo "FAIL: $label changed the log"
    else
        echo "PASS: $label (exit $actual)"
    fi
}
for executable in assistant1 assistant2 assistant1_san assistant2_san; do
    echo "=== $executable ==="
    program="$project_dir/$executable"
    test_dir=$(mktemp -d) || exit 1
    cd "$test_dir" || exit 1
    touch userlog.txt
    chmod 600 userlog.txt
    run_case 0 "Normal input" student1 "Final regression test"
    case "$executable" in
        assistant1*) missing_code=1 ;;
        assistant2*) missing_code=2 ;;
    esac
    run_case "$missing_code" "Missing arguments"
    run_case 2 "Long username" "$(printf 'A%.0s' {1..5000})" "Test"
    run_case 2 "Newline injection" student1 $'normal\nFAKE_ADMIN: changed'
    run_case 0 "Maximum lengths" "$(printf 'A%.0s' {1..64})" "$(printf 'B%.0s' {1..4096})"
    printf 'Maximum record bytes: '
    tail -n 1 userlog.txt | wc -c
done
