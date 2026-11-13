# Exatron Language & Compiler

Exatron (`.ex`) is a compiled hybrid programming and neural network description language that transpiles directly to standalone, zero-dependency C++23 source code. It unifies imperative general-purpose programming constructs—such as strongly typed scalar variables, arithmetic and logical operators, structured control flow, and functions—with native neural network primitives including statically checked multidimensional tensors, matrix multiplication (`@`), and declarative neural network models with synthesized forward-pass evaluation pipelines.

---

## Features

- **Transpiled C++23 Architecture**: Emits pure, portable C++23 source code with zero external library dependencies, compiled into high-performance native executables via GCC (`g++`).
- **Strong Static Typing**: Strict static type validation across scalars (`int`, `float`, `bool`, `string`, `void`), statically shaped tensors (`tensor<R, C>`), and model types. No implicit type coercion or silent narrowing.
- **First-Class Tensor DSL**: Built-in 1D and 2D tensor literals (`[[1.0, 2.0], [3.0, 4.0]]`), deterministic initialization functions (`zeros`, `ones`, `rand`), element-wise arithmetic, and dedicated `@` matrix multiplication with dimension compatibility checking.
- **Declarative Neural Network Models**: Concise model syntax defining input dimensions and chained layers (`Dense`) with standard activations (`relu`, `sigmoid`, `tanh`, `softmax`, `linear`), generating self-contained model structs with an optimized `forward()` evaluation pass.
- **Safe Integer Arithmetic**: Zero-division and modulo-by-zero detection at runtime with informative error reporting and controlled process termination.
- **Comprehensive Diagnostic System**: Precise line-and-column diagnostic reporting for lexical anomalies, syntax errors, type mismatches, non-exhaustive return paths, and dimension conflicts.

---

## Installation

### Supported Systems & Architectures

Exatron is designed and supported on Linux (x86_64).

| Distribution Family | Tested Distributions | Package Manager | Dependencies Managed |
|---|---|---|---|
| **Ubuntu / Debian** | Ubuntu 22.04 / 24.04+, Debian 12 / 13+, Linux Mint, Pop!_OS | `apt` | `build-essential cmake git` (plus `g++-14`/`g++-13` if default lacks C++23) |
| **Fedora / RHEL** | Fedora 39 / 40+, RHEL 9+, CentOS Stream, Rocky Linux, AlmaLinux | `dnf` | `gcc-c++ make cmake git` |
| **Arch Linux** | Arch Linux, Manjaro, EndeavourOS | `pacman` | `base-devel cmake git` |
| **Windows / macOS** | Windows (native / WSL), macOS (Intel / Apple Silicon) | N/A | *Not supported yet* (Linux only for now) |

> [!IMPORTANT]
> **Platform Support**: Only Linux is supported at this time. Running `install.sh` on non-Linux platforms (such as macOS or Windows) will abort immediately with:
> ```text
> Linux only for now
> ```

---

### One-Command Installation

To install Exatron for your user account (default prefix `~/.local`):

```bash
git clone <repo url>
cd exatron
./install.sh
```

#### Installation Variants

- **System-Wide Install** (installs to `/usr/local/bin`, invokes `sudo` only for the install step):
  ```bash
  ./install.sh --system
  ```
- **Custom Prefix Install** (installs binary to `<DIR>/bin/exatron`):
  ```bash
  ./install.sh --prefix /opt/exatron
  ```
- **Unattended Non-Interactive Install** (answers yes to dependency prompts):
  ```bash
  ./install.sh -y
  ```

---

### How the Installer Works (Step-by-Step)

The installer executes 6 structured stages:

1. **[1/6] System & Environment Detection**: Checks that the OS kernel is Linux (`uname -s`), parses `/etc/os-release` (`ID` and `ID_LIKE`), resolves target install paths, and checks user privileges.
2. **[2/6] Dependency & C++23 Feature Detection**: Searches for a C++23-capable compiler by attempting a test compilation with `-std=c++23` across candidate binaries (`g++`, `g++-16`, `g++-15`, `g++-14`, `g++-13`). Also verifies `cmake >= 3.20`, `make`, and `git`. If missing, installs the necessary distribution packages via the package manager (prompting for confirmation unless `-y`).
3. **[3/6] CMake Build Configuration**: Creates an isolated temporary build directory and configures the Release build with `-DCMAKE_BUILD_TYPE=Release`, passing the verified compiler as `-DEXATRON_CXX=<compiler>`.
4. **[4/6] Parallel Compilation**: Builds the native `exatron` compiler binary in parallel using all available CPU threads (`cmake --build <builddir> -j"$(nproc)"`).
5. **[5/6] Binary Installation**: Installs the executable to `<prefix>/bin/exatron` via `cmake --install <builddir>` (invoking `sudo` only if writing to root-owned paths like `/usr/local`).
6. **[6/6] PATH Setup & Verification**: Configures user shell startup files, runs `exatron --version`, and compiles and executes an embedded hello-world smoke test program.

#### Packages Installed
- **C++ Compiler**: GCC `g++` (version 13+ with C++23 standard library support)
- **Build Tools**: CMake (`>= 3.20`) and GNU Make
- **VCS**: Git (for cloning repository sources)

> [!NOTE]
> **Why `g++` Must Stay Installed**: Exatron is a transpiled language. The `exatron` driver converts your `.ex` source code into standalone C++23 source code embedding the `exatron_rt` tensor library, and then shells out to `g++` to compile that C++ code into a native ELF executable binary. The `g++` compiler is therefore both a build-time dependency and a permanent runtime backend requirement.

---

### Installer CLI Reference

| Flag | Description | Example |
|---|---|---|
| `--system` | Install system-wide into `/usr/local` (requires sudo only for install step) | `./install.sh --system` |
| `--prefix DIR` | Install into a custom directory prefix (default: `~/.local`) | `./install.sh --prefix /opt/exatron` |
| `-y`, `--yes` | Non-interactive mode; automatically confirm dependency package installation | `./install.sh -y` |
| `--no-deps` | Skip package manager checks and package installations | `./install.sh --no-deps` |
| `--run-tests` | Run the complete Exatron compiler test suite after build and install | `./install.sh --run-tests` |
| `--uninstall` | Cleanly remove binary, shell PATH marker blocks, and fish configuration | `./install.sh --uninstall` |
| `--dry-run` | Print every command that would be executed without modifying the system | `./install.sh --dry-run` |
| `--repo URL` | Clone and build from a remote Git repository URL | `./install.sh --repo https://github.com/exatron-lang/exatron.git` |
| `--branch NAME` | Branch or tag to check out when using `--repo` | `./install.sh --repo ... --branch main` |
| `-h`, `--help` | Display installer help message and exit | `./install.sh --help` |

#### Output of `./install.sh --help`

```text
Exatron Compiler Installer for Linux

Usage:
  ./install.sh [OPTIONS]

Options:
  --system           Install system-wide to /usr/local (requires sudo for install step)
  --prefix DIR       Install to custom prefix DIR (default: ~/.local)
  -y, --yes          Automatic yes to prompts; run non-interactively
  --no-deps          Skip package manager dependency installation
  --run-tests        Run the full test suite after building and installing
  --uninstall        Uninstall Exatron from the specified prefix
  --dry-run          Print every command that would be executed without running it
  --repo URL         Clone and build from git repository URL
  --branch NAME      Branch or tag to clone when using --repo
  -h, --help         Show this help message and exit

Environment Variables:
  EXATRON_CXX        Override C++ compiler (e.g. g++, g++-14)
  EXATRON_FORCE_DISTRO Force simulated distro in --dry-run (ubuntu, fedora, arch)
```

---

### Shell PATH Setup & Environment

For standard user installations (`~/.local` or custom `--prefix`), the installer automatically configures your shell environment:

1. Appends a single guarded block to `~/.bashrc`, `~/.zshrc`, and `~/.profile` (creating them if they do not exist):
   ```bash
   # >>> exatron >>>
   export PATH="<prefix>/bin:$PATH"
   # <<< exatron <<<
   ```
2. For Fish shell users, writes `~/.config/fish/conf.d/exatron.fish` containing:
   ```fish
   fish_add_path "<prefix>/bin"
   ```
3. **Idempotency**: Running `install.sh` multiple times safely updates the existing block and will never duplicate entries.

> [!IMPORTANT]
> **Active Terminal Sessions**: A shell script runs in a child process and cannot alter the environment variables (`PATH`) of the terminal that launched it. To start using `exatron` immediately in your current terminal, reload your shell configuration:
> ```bash
> source ~/.bashrc
> # or: export PATH="$HOME/.local/bin:$PATH"
> ```
> Or simply open a new terminal window.

---

### Verification, Updates & Uninstallation

#### Verification
Verify the installation by checking the compiler version and running a sample program:

```bash
exatron --version
```
```text
Exatron compiler v0.1.0
```

```bash
exatron docs/examples/quick_start.ex -o hello --run
```
```text
Hello from Exatron!
```

#### Updating Exatron
To update Exatron to the latest version, pull the latest changes and re-run the installer:

```bash
git pull
./install.sh
```

#### Clean Uninstallation
To remove Exatron, run the installer with `--uninstall`:

```bash
./install.sh --uninstall
```
If you installed with a custom prefix:
```bash
./install.sh --uninstall --prefix /opt/exatron
```
Uninstallation removes the `exatron` executable, cleanly removes the guarded PATH blocks from `~/.bashrc`, `~/.zshrc`, and `~/.profile`, and removes `exatron.fish`. System packages installed as dependencies remain untouched.

---

### Manual Installation (Unsupported Distributions)

If you are running an unsupported Linux distribution, install the required build dependencies manually and run CMake:

**Required Dependencies:**
- GCC `g++` (version 13+ with `-std=c++23` support)
- CMake (`>= 3.20`)
- GNU Make
- Git

**Manual Build & Install Commands:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build -j"$(nproc)"
cmake --install build
```

Then append `<prefix>/bin` to your `PATH` in `~/.bashrc`:
```bash
export PATH="$HOME/.local/bin:$PATH"
```

---

### Troubleshooting

- **`g++ too old for C++23`**: Exatron requires full C++23 support (`-std=c++23`). On older distributions like Ubuntu 22.04 LTS where the default `g++` is version 11, install a modern GCC package:
  ```bash
  sudo apt update && sudo apt install -y g++-14
  EXATRON_CXX=g++-14 ./install.sh
  ```
- **`CMake >= 3.20 is required`**: If your distribution provides an outdated CMake, install CMake from your distro's backports, via `snap install cmake --classic`, or by downloading the official precompiled binary tarball from [cmake.org](https://cmake.org/download/).
- **`exatron: command not found` immediately after install**: Your current shell session has not reloaded `PATH`. Run `source ~/.bashrc` (or `export PATH="$HOME/.local/bin:$PATH"`), or close and reopen your terminal.
- **`sudo missing or permission denied`**: Installing system-wide (`--system`) requires root privileges. If `sudo` is not available on your system, install Exatron in user mode without `--system` (installs to `~/.local/bin`), or switch to `root` (`su -`) before running `./install.sh --system`.
- **`Package manager lock errors`**: If another process (e.g. background system updates) is holding the package manager lock (`/var/lib/dpkg/lock-frontend` or `/var/lib/pacman/db.lck`), wait for the background update to complete and re-run `./install.sh`.
- **Previewing actions with `--dry-run`**: To inspect what packages and build commands would be executed without making any changes, use:
  ```bash
  ./install.sh --dry-run
  ```
  You can also preview execution on other Linux distributions using `EXATRON_FORCE_DISTRO`:
  ```bash
  EXATRON_FORCE_DISTRO=ubuntu ./install.sh --dry-run
  EXATRON_FORCE_DISTRO=fedora ./install.sh --dry-run
  EXATRON_FORCE_DISTRO=arch ./install.sh --dry-run
  ```

---

## System Requirements

- **Operating System**: Linux only (Ubuntu/Debian, Fedora/RHEL, Arch Linux; developed and verified on Arch Linux)
- **C++ Backend Compiler**: GCC `g++` 13+ supporting `-std=c++23` (tested with g++ 13, 14, 15, and 16)
- **Build System**: CMake 3.20+
- **Build Tools**: GNU Make, Git
- **Shell**: Bash 5.0+

---

## Building from Source

To build the compiler directly from source without using `install.sh`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The resulting compiler binary is generated at `build/exatron`.

---

## Quick Start

Create a minimal source file `docs/examples/quick_start.ex`:

```text
fn main() -> int {
    print("Hello from Exatron!");
    return 0;
}
```

Compile and run the program in a single step using the installed `exatron` command and the `--run` flag:

```bash
exatron docs/examples/quick_start.ex -o quick_start --run
```

**Real Terminal Output:**
```text
Hello from Exatron!
```

---

## CLI Reference

```text
Usage:
  exatron <input.ex> [-o <output_binary>] [--emit-cpp] [--dump-tokens] [--run] [-h|--help] [--version]
```

### 1. `-h`, `--help`
Displays command-line usage information and exits with code 0.

```bash
exatron --help
```
```text
Usage:
  exatron <input.ex> [-o <output_binary>] [--emit-cpp] [--dump-tokens] [--run] [-h|--help] [--version]
```

### 2. `--version`
Displays compiler version information and exits with code 0.

```bash
exatron --version
```
```text
Exatron compiler v0.1.0
```

### 3. `-o <output_binary>`
Specifies the path and filename of the compiled native executable. If omitted, the executable is created in the current working directory using the input file's stem name.

```bash
exatron docs/examples/quick_start.ex -o custom_bin
```

### 4. `--emit-cpp`
Retains the generated intermediate C++23 source file (`<output_binary>.cpp` or `<stem>.cpp`) in the filesystem and prints its generated path to standard output. Without this flag, intermediate C++ files are stored in a temporary directory and removed immediately after native compilation.

```bash
exatron docs/examples/quick_start.ex -o quick_start --emit-cpp
```
```text
Generated: quick_start.cpp
```

### 5. `--dump-tokens`
Tokenizes the input source file, prints each token's location, type, and lexeme to standard output, and exits immediately with code 0 without parsing or compiling.

```bash
exatron docs/examples/quick_start.ex --dump-tokens
```
```text
Line 1, Col 1: Fn 'fn'
Line 1, Col 4: Ident 'main'
Line 1, Col 8: LParen '('
Line 1, Col 9: RParen ')'
Line 1, Col 11: Arrow '->'
Line 1, Col 14: IntType 'int'
Line 1, Col 18: LBrace '{'
Line 2, Col 5: Print 'print'
Line 2, Col 10: LParen '('
Line 2, Col 11: StringLit 'Hello from Exatron!'
Line 2, Col 32: RParen ')'
Line 2, Col 33: Semicolon ';'
Line 3, Col 5: Return 'return'
Line 3, Col 12: IntLit '0'
Line 3, Col 13: Semicolon ';'
Line 4, Col 1: RBrace '}'
Line 5, Col 1: Eof ''
```

### 6. `--run`
Immediately executes the compiled native binary after backend compilation completes, propagating the target program's exit code.

```bash
exatron docs/examples/quick_start.ex --run
```
```text
Hello from Exatron!
```

---

## Language Tour

All examples below are available in `docs/examples/` and can be compiled and run directly.

### 1. Types and Variables (`docs/examples/tour_types_vars.ex`)
Variables are declared using the `let` keyword and require an explicit type annotation:

```text
fn main() -> int {
    let a: int = 10;
    let b: float = 3.14;
    let c: bool = true;
    let d: string = "Exatron language";
    print(a);
    print(b);
    print(c);
    print(d);
    return 0;
}
```
**Output:**
```text
10
3.14
true
Exatron language
```

### 2. Operators (`docs/examples/tour_operators.ex`)
Supports standard scalar arithmetic (`+`, `-`, `*`, `/`, `%`), comparisons (`<`, `<=`, `>`, `>=`, `==`, `!=`), and logical operators (`&&`, `||`, `!`):

```text
fn main() -> int {
    let x: int = 20;
    let y: int = 6;
    print(x + y);
    print(x - y);
    print(x * y);
    print(x / y);
    print(x % y);
    print(x > y && y > 0);
    return 0;
}
```
**Output:**
```text
26
14
120
3
2
true
```

### 3. Control Flow (`docs/examples/tour_control_flow.ex`)
Supports `if` / `else if` / `else` branches, condition-based `while` loops, and end-exclusive `for` range loops:

```text
fn main() -> int {
    let score: int = 85;
    if (score >= 90) {
        print("Grade A");
    } else if (score >= 80) {
        print("Grade B");
    } else {
        print("Grade C");
    }

    let i: int = 0;
    while (i < 3) {
        print(i);
        i = i + 1;
    }

    let total: int = 0;
    for (step in 1..4) {
        total = total + step;
    }
    print(total);

    return 0;
}
```
**Output:**
```text
Grade B
0
1
2
6
```

### 4. Functions (`docs/examples/tour_functions.ex`)
Functions are introduced with `fn`, list typed parameters, and specify return types with `-> Type`. Non-value functions specify `-> void`:

```text
fn add(a: int, b: int) -> int {
    return a + b;
}

fn greet(name: string) -> void {
    print("Welcome,");
    print(name);
}

fn main() -> int {
    greet("Student");
    let res: int = add(15, 27);
    print(res);
    return 0;
}
```
**Output:**
```text
Welcome,
Student
42
```

### 5. Tensors & Matrix Multiplication (`docs/examples/tour_tensors.ex`)
Tensors are statically shaped. Element-wise arithmetic (`+`, `-`, `*`) works between tensors of identical shapes. The `@` operator computes 2D matrix multiplication (`(M, K) @ (K, N) -> (M, N)`):

```text
fn main() -> int {
    let m1: tensor<2, 3> = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]];
    let m2: tensor<3, 2> = [[1.0, 0.0], [0.0, 1.0], [1.0, 1.0]];
    let prod: tensor<2, 2> = m1 @ m2;
    print("m1 @ m2:");
    print(prod);

    let z: tensor<2, 2> = zeros(2, 2);
    let o: tensor<2, 2> = ones(2, 2);
    print("Zeros + Ones:");
    print(z + o);
    return 0;
}
```
**Output:**
```text
m1 @ m2:
[[4.0000, 5.0000], [10.0000, 11.0000]]
Zeros + Ones:
[[1.0000, 1.0000], [1.0000, 1.0000]]
```

### 6. Neural Network Models (`docs/examples/tour_model.ex`)
Models declare input shapes and sequential `Dense` layers with activation functions. Models are instantiated with default constructors and evaluated via `.forward(...)`:

```text
model TinyMLP {
    input: tensor<1, 3>;
    layer hidden: Dense(3, 4, relu);
    layer output: Dense(4, 2, softmax);
}

fn main() -> int {
    let net: TinyMLP = TinyMLP();
    let x: tensor<1, 3> = [[0.5, -0.2, 1.0]];
    let probs: tensor<1, 2> = net.forward(x);
    print("Model predictions:");
    print(probs);
    return 0;
}
```
**Output:**
```text
Model predictions:
[[0.5157, 0.4843]]
```

---

## Neural Networks in Exatron

Exatron features a built-in declarative Neural Network Description Language (NNDL) designed for forward inference graph definition, static shape verification, and direct zero-dependency C++23 code generation.

For complete coverage—including static shape checking rules, weight initialization internals, activation function mathematics, parameter counting formulas, and 5 comprehensive end-to-end worked examples—see **[Chapter 8 of DOCUMENTATION.md](DOCUMENTATION.md#8-neural-networks-with-exatron-a-neural-network-description-language)**.

### Mini Example: Linear Regressor (`docs/examples/nn/01_linear_single_layer.ex`)

```text
// 01_linear_single_layer.ex
// A single Dense layer with linear activation: 3 inputs -> 1 output.
// This shape models a simple linear regression: y = x @ W + b.

model LinearRegressor {
    input: tensor<1, 3>;
    layer out: Dense(3, 1, linear);
}

fn main() -> int {
    let net: LinearRegressor = LinearRegressor();
    let x: tensor<1, 3> = [[1.0, 2.0, 3.0]];

    print("Linear Regressor: 3 inputs -> 1 output");
    print("Input:");
    print(x);

    let y: tensor<1, 1> = net.forward(x);
    print("Output (untrained, random weights):");
    print(y);

    return 0;
}
```

**Compile and run:**
```bash
exatron docs/examples/nn/01_linear_single_layer.ex --run
```

**Real output:**
```text
Linear Regressor: 3 inputs -> 1 output
Input:
[[1.0000, 2.0000, 3.0000]]
Output (untrained, random weights):
[[-0.3233]]
```

> [!NOTE]
> In Exatron, neural network weights are deterministically initialized with pseudo-random values via `g_rand_state`. Exatron currently generates forward evaluation pipelines; gradient backpropagation and training loops are not built in.

---

## Running the Automated Test Suite

The test suite comprises positive end-to-end execution tests, negative compile-time error checks, and runtime failure tests.

Run the test runner script:

```bash
bash tests/run_tests.sh
```

### Interpreting Test Output

```text
=== Running Positive Tests ===
  PASS: test_core
  PASS: test_nn
  PASS: test_else_if
  PASS: test_function_order
  PASS: test_literals_and_mangling

=== Running Negative Compile-Error Tests ===
  PASS: err_layer_chain
  PASS: err_matmul_dim
  PASS: err_missing_return
  PASS: err_redeclare
  PASS: err_type_mismatch
  PASS: err_undefined_var
  PASS: err_unknown_type
  PASS: err_unterminated_string

=== Running Runtime Error Tests ===
  PASS: test_div_zero

=== Summary ===
Total tests:  14
Passed tests: 14
Failed tests: 0
=== All Exatron compiler tests passed! ===
```

- **Positive Tests**: The test program is compiled and executed; its stdout is compared against a golden `.out` file in `tests/expected/`. For `test_nn`, the softmax output is validated numerically via AWK to ensure the probability distribution sums to 1.0 within 0.001.
- **Negative Tests**: Programs in `tests/errors/` must fail compilation with a non-zero exit status, and stderr must match the required error message declared in the file's `// EXPECT:` directive.
- **Runtime Error Tests**: Verifies that runtime violations (such as division by zero) terminate with status code 1 and emit the appropriate diagnostic message to stderr.

---

## Project Structure

```text
exatron/
├── CMakeLists.txt              # CMake build configuration for C++23 with strict warnings
├── include/
│   ├── token.hpp               # Token types, source locations, and token string conversion
│   ├── lexer.hpp               # Lexical analyzer interface
│   ├── ast.hpp                 # Abstract Syntax Tree nodes and TypeInfo structure
│   ├── parser.hpp              # Recursive descent parser interface
│   ├── type_checker.hpp        # Symbol table, type verification, and semantic analyzer
│   ├── runtime_emitter.hpp     # Embedded zero-dependency C++23 tensor/NN runtime library
│   └── codegen.hpp             # C++23 transpiler code generator interface
├── src/
│   ├── main.cpp                # Compiler driver orchestrating pipeline and g++ invocation
│   ├── lexer.cpp               # Lexer implementation with escape sequences and comments
│   ├── parser.cpp              # Recursive descent parser implementing full language grammar
│   ├── type_checker.cpp        # Type inference, return path analysis, and dimension checks
│   └── codegen.cpp             # C++23 AST transpiler with ex_ name mangling
├── tests/
│   ├── test_core.ex            # Core imperative language test suite
│   ├── test_nn.ex              # Neural network DSL and matrix multiplication test
│   ├── test_else_if.ex         # Chained else-if conditional branching test
│   ├── test_function_order.ex  # Forward declaration and call order test
│   ├── test_literals_and_mangling.ex # Keyword collision and literal emission test
│   ├── test_div_zero.ex        # Division by zero runtime safety test
│   ├── run_tests.sh            # Automated regression test harness
│   ├── expected/               # Golden output reference files for stdout diffing
│   └── errors/                 # Negative compile-error tests with EXPECT annotations
└── docs/
    ├── nn_capability_audit.md  # Comprehensive neural network capability audit
    └── examples/               # Runnable source examples referenced in documentation
        └── nn/                 # Neural network worked examples (01 to 05)
```

---

## Compiler Architecture & Pipeline

```text
Source Code (.ex)
      │
      ▼
┌───────────────┐
│     Lexer     │  Scans characters into a flat std::vector<Token> with SourceLoc
└───────┬───────┘
        ▼
┌───────────────┐
│    Parser     │  Constructs polymorphic AST (exatron::Program) via recursive descent
└───────┬───────┘
        ▼
┌───────────────┐
│ Type Checker  │  Validates symbols, scopes, return paths, dimensions, and types
└───────┬───────┘
        ▼
┌───────────────┐
│ Code Generator│  Transpiles AST into self-contained C++23 with exatron_rt runtime
└───────┬───────┘
        ▼
┌───────────────┐
│  g++ Backend  │  Invokes g++ -std=c++23 -O2 to produce native executable binary
└───────┬───────┘
        ▼
Executable Binary
```

1. **Lexical Analysis (`src/lexer.cpp`)**: Converts UTF-8 source into categorized tokens with exact line and column tracking, stripping whitespace and comments while preserving literal payloads.
2. **Parsing (`src/parser.cpp`)**: Builds an AST using precedence climbing for expressions and recursive descent for statements and declarations.
3. **Semantic Analysis & Type Checking (`src/type_checker.cpp`)**: Validates lexical scopes, ensures static tensor rank and dimension compatibility, enforces exhaustive return paths on non-void functions, and attaches `TypeInfo` annotations to AST nodes.
4. **Code Generation (`src/codegen.cpp`)**: Prepends the self-contained `exatron_rt` runtime header, mangles user identifiers with `ex_` to prevent collisions with C++ reserved words, and emits idiomatic C++23 source code.
5. **Backend Compilation (`src/main.cpp`)**: Spawns `g++ -std=c++23 -O2` using sanitized shell parameters to link native machine code.

---

## Common Compiler Errors & How to Fix Them

### 1. Unterminated String Literal (`[Lexer Error]`)
- **Code**: `let s: string = "hello;` (`docs/examples/err_sample_lexer.ex`)
- **Diagnostic**: `[Lexer Error] Line 2, Col 23: Unterminated string literal`
- **Fix**: Ensure every opening double quote has a matching closing quote on the same line.

### 2. Tensor Dimension Overflow (`[Parse Error]`)
- **Code**: `let t: tensor<1, 2, 3> = [[1.0]];` (`docs/examples/err_sample_parse.ex`)
- **Diagnostic**: `[Parse Error] Line 2, Col 25: Tensors support at most 2 dimensions`
- **Fix**: Exatron tensors support 1D (`tensor<N>`, normalized to `tensor<1, N>`) or 2D (`tensor<R, C>`). Reduce the dimensions to rank 1 or 2.

### 3. Strict Scalar Type Mismatch (`[Type Error]`)
- **Code**: `let x: int = 4.5;` (`docs/examples/err_sample_type.ex`)
- **Diagnostic**: `[Type Error] Line 2, Col 5: Type mismatch in let declaration: declared int but initializer has type float`
- **Fix**: Exatron forbids implicit numerical conversion. Change the variable type to `float` or use an integer literal `4`.

### 4. Matrix Multiplication Dimension Mismatch (`[Type Error]`)
- **Code**: `let c: tensor<2, 2> = a @ b;` where `a` is `tensor<2, 3>` and `b` is `tensor<4, 2>` (`tests/errors/err_matmul_dim.ex`)
- **Diagnostic**: `[Type Error] Line 5, Col 27: Matrix multiplication dimension mismatch: cannot multiply tensor<2, 3> by tensor<4, 2>`
- **Fix**: Ensure the inner dimension matches: for `tensor<M, K> @ tensor<K2, N>`, `K` must equal `K2`.

### 5. Missing Return Statement on Code Path (`[Type Error]`)
- **Code**: `fn f(x: int) -> int { if (x > 0) { return 1; } }` (`tests/errors/err_missing_return.ex`)
- **Diagnostic**: `[Type Error] Line 2, Col 1: Function 'f' might not return a value on all paths`
- **Fix**: Add a terminal `return` statement or provide an exhaustive `else` branch returning an integer.

### 6. Division by Zero (`Runtime Error`)
- **Code**: `let res: int = a / b;` where `b == 0` (`docs/examples/err_sample_runtime.ex`)
- **Diagnostic**: `Runtime Error: division by zero`
- **Fix**: Guard integer division and modulo operations with conditionals checking that the divisor is non-zero.

---

## Known Limitations

- **Forward-Only Models**: Neural network models support inference forward passes only. Backward passes, loss functions, autograd, and training loops are not currently built in.
- **Fixed Model Batch Size**: Model input shapes are fixed statically by the model's `input:` declaration (dynamic batch dimensions are unsupported).
- **Rank 1 and 2 Tensors Only**: Tensors are limited to 1D vectors and 2D matrices. Higher-dimensional tensors (3D, 4D batches for computer vision) are unsupported.
- **No Implicit Coercion**: Integers and floats do not implicitly promote; mixing types in binary operators or assignments produces a compile-time error.
- **Single-Error Termination**: Compilation halts immediately upon encountering the first lexical, syntax, or type error.

---

## Roadmap

1. **Training & Automatic Differentiation**: Reverse-mode autograd tape with gradient calculation and optimizer updates for `model` declarations.
2. **Dynamic Tensor Shapes & Batches**: Dimension parameterization (`tensor<?, 128>`) to support variable batch sizes.
3. **Multi-Error Recovery**: Enhanced parser and semantic analyzer synchronization to collect and display multiple diagnostics per compilation pass.
4. **Higher Rank Tensors**: Extension of `exatron_rt::Tensor` to support arbitrary rank dimensions (`tensor<N, C, H, W>`) with strides and slicing.

---

## Project Metadata

- **Author**: <AUTHOR>
- **Institution**: <COLLEGE>
- **Language Standard**: C++23 (`-std=c++23`)
- **License**: Educational / Open Source

