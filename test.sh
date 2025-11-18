#!/usr/bin/env bash

if [[ -z "$1" ]]; then
    echo "Usage: $0 <test-number>"
    exit 1
fi

TESTDIR="assign-2-tests/ts$1"

for ast in "$TESTDIR"/*.astj; do
    base=$(basename "$ast" .astj)
    soln="$TESTDIR/$base.soln"

    output=$(./type "$ast")

    diff_output=$(diff <(echo "$output") "$soln")

    if [[ $? -eq 0 ]]; then
        echo "passed: $base"
    else
        echo "failed: $base"
        echo "----- diff -----"
        echo "$diff_output"
        echo "----------------"
    fi
done
