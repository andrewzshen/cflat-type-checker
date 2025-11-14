#!/usr/bin/env bash

TESTDIR="assign-2-tests/ts1"

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
