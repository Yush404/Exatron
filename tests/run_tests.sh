#!/usr/bin/env bash
set -uo pipefail

cmake -B build
cmake --build build -j

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

pass() {
    local name="$1"
    echo "  PASS: $name"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

fail() {
    local name="$1"
    local reason="$2"
    echo "  FAIL: $name ($reason)"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

echo "=== Running Positive Tests ==="

POSITIVE_TESTS=(
    "test_core"
    "test_nn"
    "test_else_if"
    "test_function_order"
    "test_literals_and_mangling"
)

for test_name in "${POSITIVE_TESTS[@]}"; do
    src_file="tests/${test_name}.ex"
    expected_file="tests/expected/${test_name}.out"
    bin_file="build/${test_name}"

    if [[ ! -f "$src_file" ]]; then
        fail "$test_name" "source file $src_file not found"
        continue
    fi
    if [[ ! -f "$expected_file" ]]; then
        fail "$test_name" "expected file $expected_file not found"
        continue
    fi

    actual_output=$(./build/exatron "$src_file" -o "$bin_file" --run 2>&1)
    status=$?

    if [[ $status -ne 0 ]]; then
        fail "$test_name" "non-zero exit code $status"
        continue
    fi

    # Normalize test_nn softmax probability line
    if [[ "$test_name" == "test_nn" ]]; then
        actual_output=$(awk '
        BEGIN { check_next=0 }
        /IrisNet Softmax Output Probabilities/ {
            print;
            check_next=1;
            next;
        }
        check_next==1 {
            check_next=0;
            line=$0;
            gsub(/\[|\]/, "", line);
            n = split(line, arr, ",");
            sum = 0.0;
            for (i = 1; i <= n; i++) sum += arr[i];
            diff = sum - 1.0;
            if (diff < 0) diff = -diff;
            if (diff <= 0.001) print "PROBS_OK";
            else print "PROBS_BAD";
            next;
        }
        { print }
        ' <<< "$actual_output")
    fi

    diff_out=$(diff -u "$expected_file" <(echo "$actual_output"))
    if [[ $? -eq 0 ]]; then
        pass "$test_name"
    else
        fail "$test_name" "output mismatch"
        echo "$diff_out"
    fi
done

echo ""
echo "=== Running Neural Network Documentation Examples ==="

NN_TESTS=(
    "nn_01_linear_single_layer:docs/examples/nn/01_linear_single_layer.ex"
    "nn_02_binary_classifier:docs/examples/nn/02_binary_classifier.ex"
    "nn_03_multiclass_iris_shape:docs/examples/nn/03_multiclass_iris_shape.ex"
    "nn_04_wide_vs_deep:docs/examples/nn/04_wide_vs_deep.ex"
    "nn_05_hybrid_pipeline:docs/examples/nn/05_hybrid_pipeline.ex"
)

for entry in "${NN_TESTS[@]}"; do
    test_name="${entry%%:*}"
    src_file="${entry##*:}"
    expected_file="tests/expected/${test_name}.out"
    bin_file="build/${test_name}"

    if [[ ! -f "$src_file" ]]; then
        fail "$test_name" "source file $src_file not found"
        continue
    fi
    if [[ ! -f "$expected_file" ]]; then
        fail "$test_name" "expected file $expected_file not found"
        continue
    fi

    actual_output=$(./build/exatron "$src_file" -o "$bin_file" --run 2>&1)
    status=$?

    if [[ $status -ne 0 ]]; then
        fail "$test_name" "non-zero exit code $status"
        continue
    fi

    # Normalize probability lines (softmax/sigmoid outputs may drift across compilers)
    normalize_probs() {
        awk '{
            line=$0;
            # Match lines like [[0.3289, 0.3968, 0.2743]]
            if (match(line, /\[\[[-0-9., ]+\]\]/)) {
                inner = line;
                gsub(/\[|\]/, "", inner);
                n = split(inner, arr, ",");
                all_prob = 1;
                sum = 0.0;
                for (i = 1; i <= n; i++) {
                    gsub(/ /, "", arr[i]);
                    v = arr[i] + 0.0;
                    if (v < -10.0 || v > 10.0) all_prob = 0;
                    sum += v;
                }
                # If looks like probability row (sum near 1 and all in [0,1])
                diff = sum - 1.0;
                if (diff < 0) diff = -diff;
                if (diff <= 0.01 && all_prob) {
                    print line;
                } else {
                    print line;
                }
            } else {
                print line;
            }
        }' <<< "$1"
    }

    diff_out=$(diff -u "$expected_file" <(echo "$actual_output"))
    if [[ $? -eq 0 ]]; then
        pass "$test_name"
    else
        fail "$test_name" "output mismatch"
        echo "$diff_out"
    fi
done

echo ""
echo "=== Running Negative Compile-Error Tests ==="

for err_file in tests/errors/*.ex; do
    test_name=$(basename "$err_file" .ex)
    expected_substr=$(sed -n '1s/^\/\/ EXPECT: //p' "$err_file")

    if [[ -z "$expected_substr" ]]; then
        fail "$test_name" "missing // EXPECT: header in $err_file"
        continue
    fi

    err_output=$(./build/exatron "$err_file" -o "build/${test_name}" 2>&1)
    status=$?

    if [[ $status -eq 0 ]]; then
        fail "$test_name" "expected compile failure but got exit code 0"
        continue
    fi

    if grep -qF "$expected_substr" <<< "$err_output"; then
        pass "$test_name"
    else
        fail "$test_name" "expected '$expected_substr' in output, got: $err_output"
    fi
done

echo ""
echo "=== Running Runtime Error Tests ==="

div_zero_src="tests/test_div_zero.ex"
div_zero_bin="build/test_div_zero"
if [[ -f "$div_zero_src" ]]; then
    div_output=$(./build/exatron "$div_zero_src" -o "$div_zero_bin" --run 2>&1)
    div_status=$?

    if [[ $div_status -eq 1 ]] && grep -qF "division by zero" <<< "$div_output"; then
        pass "test_div_zero"
    else
        fail "test_div_zero" "expected exit code 1 with 'division by zero', got exit=$div_status, output: $div_output"
    fi
else
    fail "test_div_zero" "$div_zero_src not found"
fi

echo ""
echo "=== Summary ==="
echo "Total tests:  $TOTAL_TESTS"
echo "Passed tests: $PASSED_TESTS"
echo "Failed tests: $FAILED_TESTS"

if [[ $FAILED_TESTS -eq 0 ]]; then
    echo "=== All Exatron compiler tests passed! ==="
    exit 0
else
    echo "=== Some tests failed! ==="
    exit 1
fi
