# Feature 07: Compiler CLI Driver, End-to-End Pipeline & Automated Test Suite

## 1. Objective
Complete the Exatron compiler CLI driver in `src/main.cpp` to orchestrate the full pipeline (Lexer -> Parser -> TypeChecker -> CodeGenerator -> native binary invocation via `g++`), create comprehensive `.ex` test programs in `tests/`, and run an automated verification script `tests/run_tests.sh`.

## 2. CLI Driver Specification (`src/main.cpp`)
Implement `src/main.cpp` to support the following command-line interface:

```text
Usage:
  exatron <input.ex> [-o <output_binary>] [--emit-cpp] [--dump-tokens] [--run]
```

### Supported Flags & Behavior
1. **File Reading**:
   - Read the input `.ex` file into a `std::string`. If the file cannot be opened, print a clear error to `stderr` and return exit code `1`.
2. **Pipeline Execution**:
   - **Step 1 (Lexing)**: Instantiate `exatron::Lexer(source)` and call `tokenize()`.
     - If `--dump-tokens` is passed, print each token's line, column, type name, and lexeme, then exit with `0`.
   - **Step 2 (Parsing)**: Instantiate `exatron::Parser(tokens)` and call `parse_program()`.
   - **Step 3 (Type Checking)**: Instantiate `exatron::TypeChecker()` and call `check_program(program)`.
   - **Step 4 (Code Generation)**: Instantiate `exatron::CodeGenerator()` and call `generate(program)` to get the C++23 code string.
     - Always write the generated C++ source to `<output_binary>.cpp` (default `<stem>.cpp`, or `/tmp/exatron_out.cpp` if not `--emit-cpp`).
     - If `--emit-cpp` is passed without `-o`, write `<stem>.cpp` in the current working directory and print its path.
   - **Step 5 (Native Compilation via `g++`)**:
     - Invoke `g++ -std=c++23 -O2 <generated.cpp> -o <output_binary>` using `std::system(...)`.
     - If `g++` returns non-zero, report a backend compilation failure and return `1`.
   - **Step 6 (Optional Immediate Execution)**:
     - If `--run` is passed, immediately execute `./<output_binary>` and return its exit code.
3. **Diagnostic Catch Blocks**:
   - Catch `LexerError`, `ParseError`, and `TypeError`, print their formatted message to `stderr`, and exit with code `1`.

## 3. End-to-End Test Suite (`tests/`)

### 3.1 Test 1: General-Purpose Core (`tests/test_core.ex`)
Create `tests/test_core.ex` to test variables, arithmetic, recursion/functions, `if/else`, `while`, and `for (i in a..b)`:

```text
// General-purpose language test for Exatron
fn factorial(n: int) -> int {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

fn main() -> int {
    let base: int = 5;
    let fact5: int = factorial(base);
    print("Factorial of 5:");
    print(fact5);

    let sum: int = 0;
    for (i in 1..6) {
        sum = sum + i;
    }
    print("Sum 1..5:");
    print(sum);

    let count: int = 3;
    while (count > 0) {
        count = count - 1;
    }
    print("Countdown finished:");
    print(count);

    return 0;
}
```

### 3.2 Test 2: Hybrid Neural Network & Tensor DSL (`tests/test_nn.ex`)
Create `tests/test_nn.ex` to test 2D tensor literals, `@` matrix multiplication, `zeros`/`ones`/`rand`, and a multi-layer `model` forward pass:

```text
// Hybrid Neural Network & Tensor DSL test for Exatron
model IrisNet {
    input: tensor<1, 4>;
    layer hidden1: Dense(4, 8, relu);
    layer hidden2: Dense(8, 4, tanh);
    layer output_layer: Dense(4, 3, softmax);
}

fn main() -> int {
    let a: tensor<2, 3> = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]];
    let b: tensor<3, 2> = [[0.5, 1.0], [1.5, 2.0], [2.5, 3.0]];
    let c: tensor<2, 2> = a @ b;

    print("Matrix multiplication (2x3 @ 3x2 -> 2x2):");
    print(c);

    let net: IrisNet = IrisNet();
    let sample: tensor<1, 4> = [[5.1, 3.5, 1.4, 0.2]];
    let probs: tensor<1, 3> = net.forward(sample);

    print("IrisNet Softmax Output Probabilities (1x3):");
    print(probs);

    return 0;
}
```

### 3.3 Automated Test Runner (`tests/run_tests.sh`)
Create `tests/run_tests.sh` (and `chmod +x tests/run_tests.sh`):

```bash
#!/usr/bin/env bash
set -euo pipefail

cmake -B build
cmake --build build -j

echo "=== Running Test 1: Core Language (test_core.ex) ==="
./build/exatron tests/test_core.ex -o build/test_core --emit-cpp --run

echo "=== Running Test 2: Neural Network DSL (test_nn.ex) ==="
./build/exatron tests/test_nn.ex -o build/test_nn --emit-cpp --run

echo "=== All Exatron compiler tests passed! ==="
```

## 4. Final Verification Checklist
Run `bash tests/run_tests.sh` and confirm:
1. The compiler builds with zero errors under C++23 (`g++ 16+`, `cmake 3.20+`).
2. `test_core.ex` prints `120` for `factorial(5)`, `15` for `sum`, and `0` for `count`.
3. `test_nn.ex` computes the `2x2` matrix multiplication `[[11.0000, 14.0000], [26.0000, 32.0000]]` and outputs a valid `1x3` probability distribution from `IrisNet.forward(sample)` whose elements sum to `1.0`.
