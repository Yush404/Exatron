# Exatron: Corrections and Hardening Addendum

**Status:** the compiler was already built from specs 01 to 07. This file patches defects found in review. Where it conflicts with an older spec, THIS FILE WINS.

## How to apply (mandatory process)

1. **Audit first.** For every item below, inspect the existing code. If it is already correct, mark it `ALREADY-OK` and move on. Do not rewrite working code.
2. Work through **groups A to E in order**. After each group:
   - rebuild with `cmake -B build && cmake --build build -j` (zero errors, zero warnings under `-Wall -Wextra -Wpedantic`),
   - run the programs relevant to that group,
   - fix every problem before starting the next group. If a group cannot be completed, stop and report. Do not work around it.
3. Do not rename public classes, files, or the directory layout. Do not refactor unrelated code.
4. Source files keep the `.ex` extension.
5. Finish with a report table: item ID, status (`FIXED` / `ALREADY-OK` / `SKIPPED` + reason), files changed. List every place this file was ambiguous and what you decided.

---

## GROUP A: Build and lexer

### A1. CMake keyword case
`cmake_minimum_required` must use the uppercase keyword: `cmake_minimum_required(VERSION 3.20)`. CMake keywords are case-sensitive. Keep `file(GLOB_RECURSE ... CONFIGURE_DEPENDS ...)` as is.

### A2. Lexer end-of-file token and token names
- `Lexer::tokenize()` must always append a final `Token{TokenType::Eof, "", <location at end of input>}`. The parser relies on it.
- `token_type_to_string(TokenType)` must be implemented (in `src/lexer.cpp`) with a `switch` over EVERY enumerator and no `default:` branch, returning the exact enumerator name as a string (`"Let"`, `"MatMul"`, ...).
- **Check:** `./build/exatron tests/test_core.ex --dump-tokens` lists tokens with correct line and column, and the last line shows `Eof`.

---

## GROUP B: Parser

### B1. Support `else if`
New grammar rule:
```ebnf
IfStmt ::= "if" "(" Expr ")" Block ( "else" ( IfStmt | Block ) )?
```
Implementation constraint: do NOT change the AST. In `parse_if_stmt`, when `else` is followed by `if`, parse the nested `if` recursively and wrap the resulting `IfStmt` in a synthetic `BlockStmt` containing exactly that one statement, and use that block as `else_branch`. The type checker and code generator then need no special handling.

### B2. Normalize tensor shapes at parse time
In `parse_type`, `tensor<N>` becomes shape `{1, N}` and `tensor<R, C>` stays `{R, C}`. More than two dimensions is a `ParseError`: `Tensors support at most 2 dimensions`. Every dimension must be greater than zero. Keep the defensive normalization already present in the type checker.

---

## GROUP C: Type checker

Every error uses the existing format: `[Type Error] Line <L>, Col <C>: <message>`.

### C1. Identifier expressions
`IdentExpr` must look the name up through the scope stack, set `inferred_type`, and report `Undefined variable '<name>'` when missing. Using a function name as a value is an error.

### C2. Unary expressions
- `-` applies to `int` (result `int`) or `float` (result `float`).
- `!` applies to `bool` (result `bool`).
- Anything else: `Operator '<op>' cannot be applied to <type>`.
- Set `inferred_type` on the node.

### C3. Return-path checking
- A non-`void` function must return on every path. Rule: a block "always returns" if its LAST statement always returns. A `ReturnStmt` always returns. An `IfStmt` with an `else` always returns when BOTH branch blocks always return. `while` and `for` never count. Error: `Function '<name>' might not return a value on all paths`.
- In a `void` function, `return <expr>;` is an error. In a non-`void` function, a bare `return;` is an error.
- `main` must be exactly `fn main() -> int` with no parameters.

### C4. Unknown type names
Any `BaseType::Model` type used in a `let`, parameter, or return type must name a declared model. Error: `Unknown type '<name>'`. Models must be registered before function signatures are validated.

### C5. Duplicate declarations
Error `Variable '<name>' is already declared in this scope` for a duplicate in the same scope (shadowing an OUTER scope is allowed). Also reject duplicate function names, model names, layer names inside one model, and parameter names inside one function.

### C6. Call checking
- Unknown callee: `Undefined function '<name>'`.
- Wrong argument count: `Function '<name>' expects <N> argument(s) but got <M>`.
- Wrong argument type: name the parameter position, expected type, and actual type.

### C7. Mismatch messages
Assignment, `let`, `return`, and binary-operator type mismatches must print BOTH types, for example `Cannot assign float to variable 'x' of type int`. There is no implicit int-to-float conversion anywhere. Mixing them is an error that shows both types.

---

## GROUP D: Code generation and runtime

### D1. Literal emission
- `IntLitExpr` -> `static_cast<int64_t>(<N>)`.
- `FloatLitExpr` -> format with `std::setprecision(9)`; if the text contains none of `.`, `e`, `E`, append `.0`; then append `f` (for example `0.5f`, `2.0f`).
- `BoolLitExpr` -> `true` / `false`.
- `StringLitExpr` -> `std::string("...")`. Escape `\\`, `"`, `\n`, `\t`; emit any other non-printable byte as a 3-digit octal escape (`\NNN`).
- `UnaryExpr` -> `(-<operand>)` / `(!<operand>)`.
- Every element of a tensor literal is wrapped in `static_cast<float>(<expr>)` to avoid narrowing errors in the initializer list.

**Why:** a bare `"text"` picks the `bool` overload of `print_val`, and a bare `5` is ambiguous between `int64_t`, `float`, and `bool`.

### D2. Forward declarations
After the model structs, emit a prototype for every function except `main`, then emit all definitions in source order. A function must be callable before it appears in the source file.

### D3. Name mangling
Prefix every user-defined name in generated C++ with `ex_`: variables, parameters, loop variables, functions, model type names, and layer member names. A call whose callee is a model emits `ex_<Model>()`. Exceptions: `main` (emitted as `int main()`), the method name `forward`, and everything in namespace `exatron_rt`. Error messages shown to the user always use the ORIGINAL names. This prevents clashes with C++ keywords and library names (`new`, `class`, `delete`, `abs`, `exit`, ...).

### D4. `for` loops evaluate the end bound once
```cpp
for (int64_t ex_i = <start>, ex_end_N = <end>; ex_i < ex_end_N; ++ex_i) { ... }
```
`N` is a unique counter kept in `CodeGenerator`. The range `a..b` is end-exclusive.

### D5. Integer division and modulo safety
Add to the runtime `exatron_rt::idiv(int64_t, int64_t)` and `exatron_rt::imod(int64_t, int64_t)`. If the divisor is `0`, write `Runtime Error: division by zero` to `stderr` and call `std::exit(1)`. Emit them for `int / int` and `int % int`. Float division is emitted directly.

### D6. Deterministic random initialization
`Tensor::rand` must use ONE shared generator state, for example `inline uint32_t g_rand_state = 1337;`, advanced on every draw:
```cpp
g_rand_state = g_rand_state * 1664525u + 1013904223u;
float v = static_cast<float>(g_rand_state >> 8) / 16777216.0f - 0.5f;   // [-0.5, 0.5)
```
Different layers then get different weights, and every run gives identical results.

### D7. Printing
- Integers print plainly, floats with default `std::cout` formatting, bools as `true` / `false`, strings as raw text, always followed by a newline.
- A tensor prints as `[[a, b], [c, d]]` with every element at 4 decimal places. Build the text in a local `std::ostringstream`, so `std::fixed` and precision never leak into later prints.
- Add a `print_val(const char*)` overload that forwards to the `std::string` version, as a safety net.

---

## GROUP E: Driver and tests

### E1. Driver hardening (`src/main.cpp`)
- Use `std::filesystem` for stems and paths.
- Quote every path passed to `std::system` with a `shell_quote()` helper (wrap in single quotes, escape embedded single quotes).
- Decode `std::system` results with `WIFEXITED` / `WEXITSTATUS` (`<sys/wait.h>`). `g++` failure returns exit code 1 with a clear message. `--run` returns the compiled program's own exit code.
- If `--emit-cpp` is given, keep the generated source at `<output>.cpp` (or `<stem>.cpp`) and print its path. Otherwise write it to a unique file under `std::filesystem::temp_directory_path()` and delete it after compiling. Never use a fixed `/tmp/exatron_out.cpp`.
- With no `-o`, the binary is named `<stem>` in the current directory. `--run` executes the ABSOLUTE path of the binary (`std::filesystem::absolute`).
- Add `-h` / `--help` printing the usage text. An unknown flag prints the usage text and exits with code 1.

### E2. Fix the wrong expected value in the old spec
For `a @ b` in `tests/test_nn.ex` the correct result is `[[11.0000, 14.0000], [24.5000, 32.0000]]` (second row: 4*0.5 + 5*1.5 + 6*2.5 = 24.5). The old spec's `26.0` was wrong. Trust the arithmetic, not the old text.

### E3. Golden-output tests
Create `tests/expected/<name>.out` for every positive test and make the runner diff the program's stdout against it.

`tests/expected/test_core.out`:
```text
Factorial of 5:
120
Sum 1..5:
15
Countdown finished:
0
```

`tests/expected/test_nn.out` (the probability line is normalized by the runner, see E5):
```text
Matrix multiplication (2x3 @ 3x2 -> 2x2):
[[11.0000, 14.0000], [24.5000, 32.0000]]
IrisNet Softmax Output Probabilities (1x3):
PROBS_OK
```

### E4. New positive tests (write exactly these programs)

`tests/test_else_if.ex`
```text
fn classify(n: int) -> int {
    if (n < 0) {
        return -1;
    } else if (n == 0) {
        return 0;
    } else if (n < 10) {
        return 1;
    } else {
        return 2;
    }
}

fn main() -> int {
    print(classify(-5));
    print(classify(0));
    print(classify(7));
    print(classify(99));
    return 0;
}
```
Expected output: `-1`, `0`, `1`, `2` (one per line).

`tests/test_function_order.ex`
```text
fn main() -> int {
    print(double_it(21));
    print(is_even(10));
    return 0;
}

fn double_it(x: int) -> int {
    return x * 2;
}

fn is_even(x: int) -> bool {
    return x % 2 == 0;
}
```
Expected output: `42`, `true`.

`tests/test_literals_and_mangling.ex`
```text
fn abs(x: int) -> int {
    if (x < 0) {
        return -x;
    }
    return x;
}

fn main() -> int {
    let new: int = abs(-7);
    let class: float = 2.5;
    let delete: bool = new > 3;
    print("literals:");
    print(5);
    print(class);
    print(delete);
    print(new);
    print(-class);
    return 0;
}
```
Expected output: `literals:`, `5`, `2.5`, `true`, `7`, `-2.5`.

Save each expected output as `tests/expected/<name>.out`.

### E5. Negative and runtime tests
- **Compile-error tests** go in `tests/errors/`. Each file's FIRST line is `// EXPECT: <substring>`. The runner compiles the file, requires a NON-ZERO exit code, and requires `<substring>` in stderr. Write these files:
  - `err_matmul_dim.ex`: multiply `tensor<2, 3>` by `tensor<4, 2>`. EXPECT `Matrix multiplication dimension mismatch`.
  - `err_layer_chain.ex`: model with `Dense(4, 8, relu)` then `Dense(6, 2, softmax)`. EXPECT `expects input dimension 6`.
  - `err_undefined_var.ex`: uses a variable never declared. EXPECT `Undefined variable`.
  - `err_missing_return.ex`: a non-void function whose body has only an `if` without `else` returning. EXPECT `might not return a value`.
  - `err_type_mismatch.ex`: `let x: int = 1.5;`. EXPECT `Type Error`.
  - `err_redeclare.ex`: the same name declared twice in one scope. EXPECT `already declared`.
  - `err_unknown_type.ex`: `fn f(m: Ghost) -> void { }` plus a valid `main`. EXPECT `Unknown type`.
  - `err_unterminated_string.ex`: a string literal missing its closing quote. EXPECT `Lexer Error`.
- **Runtime-error test:** `tests/test_div_zero.ex` computes `10 / 0` using two `int` variables and prints the result. The runner requires exit code 1 and `division by zero` in stderr.

### E6. Rewrite `tests/run_tests.sh`
- Build first (`cmake -B build && cmake --build build -j`).
- For each positive test: compile with `./build/exatron tests/<name>.ex -o build/<name> --run`, capture stdout, `diff` against `tests/expected/<name>.out`.
- For `test_nn` only, normalize the line after `IrisNet Softmax Output Probabilities (1x3):` before diffing. Strip brackets, split on commas, and sum the numbers with `awk`. Replace the line with `PROBS_OK` if the sum is within 0.001 of 1.0, otherwise `PROBS_BAD`. Never compare the random-dependent digits exactly.
- Run the negative and runtime tests as described in E5.
- Print `PASS` or `FAIL` per test and a final summary count. Exit non-zero if anything failed.

**Final acceptance:** `bash tests/run_tests.sh` ends with every test `PASS` and exit code 0.

---

## NON-GOALS (document these as known limitations; do NOT implement now)

- Words such as `input`, `layer`, `linear`, `relu`, `sigmoid`, `tanh`, `softmax`, `zeros`, `ones`, `rand`, `Dense`, `model`, and `print` are reserved and cannot be used as identifiers.
- No implicit `int` to `float` conversion and no string concatenation.
- Tensors have rank 1 or 2 only (rank 1 is stored as `{1, N}`). Element-wise tensor `/` is not supported.
- Models support a forward pass only. There is no training or backpropagation. Weights are random but deterministic.
- A model's input shape is fixed by its `input:` declaration, so there is no dynamic batch size.
- Only the first lexical, parse, or type error is reported.
