#!/usr/bin/env bash

if [[ -z "$1" ]]; then
    echo "Usage: $0 <test-number>"
    exit 1
fi

TESTS_DIR="assign-2-tests/ts$1"

if [[ ! -d "$TESTS_DIR" ]]; then
    echo "Error: directory '$TESTS_DIR' does not exist"
    exit 1
fi

TESTS=( $(ls "$TESTS_DIR"/*.astj | sort -V) )

for ast in "${TESTS[@]}"; do
    base=$(basename "$ast" .astj)
    soln="$TESTS_DIR/$base.soln"

    output=$(./type "$ast")

    diff_output=$(diff <(echo "$output") "$soln")

    if [[ $? -eq 0 ]]; then
        echo "passed: $base"
    else
        echo "FAILED: $base"
        echo "----- diff -----"
        echo "$diff_output"
        echo "----------------"
    fi
done
