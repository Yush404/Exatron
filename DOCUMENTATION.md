# Exatron Language Manual & Specification

---

## Table of Contents
- [1. Introduction & Design Goals](#1-introduction--design-goals)
- [2. Lexical Structure](#2-lexical-structure)
- [3. Type System](#3-type-system)
- [4. Declarations & Statements](#4-declarations--statements)
- [5. Functions](#5-functions)
- [6. Expressions & Precedence](#6-expressions--precedence)
- [7. Tensors & Matrix Multiplication](#7-tensors--matrix-multiplication)
- [8. Neural Networks with Exatron: A Neural Network Description Language](#8-neural-networks-with-exatron-a-neural-network-description-language)
- [9. Built-in Functions & Printing Rules](#9-built-in-functions--printing-rules)
- [10. Installation & Toolchain](#10-installation--toolchain)
- [11. Compiler CLI Reference](#11-compiler-cli-reference)
- [12. Compilation Model & Runtime Architecture](#12-compilation-model--runtime-architecture)
- [13. Error Reference & Diagnostics](#13-error-reference--diagnostics)
- [14. Formal EBNF Grammar](#14-formal-ebnf-grammar)
- [15. Known Limitations](#15-known-limitations)
- [16. Complete Example Programs](#16-complete-example-programs)

---

## 1. Introduction & Design Goals

**Exatron** is a compiled, statically typed language designed to combine imperative systems programming with a first-class domain-specific language (DSL) for tensor computation and neural network modeling. Exatron transpiles into high-performance, standalone C++23 source code that compiles directly to native binaries using GCC (`g++`).

### Primary Design Goals
1. **Expressive Hybrid Syntax**: Seamlessly mix classic imperative logic (variables, loops, conditional logic, functions) with mathematical tensor operations and neural network model definitions.
2. **Compile-Time Geometric Safety**: Tensor ranks and matrix multiplication dimensions are verified at compile time, eliminating runtime dimension-mismatch errors during neural network evaluation.
3. **Zero-Dependency Output**: The compiler emits self-contained C++23 files containing an embedded runtime (`exatron_rt`) with zero external library dependencies.
4. **Predictability & Determinism**: Strict static typing without implicit scalar conversions, combined with deterministic pseudorandom initialization across execution environments.

---

## 2. Lexical Structure

### 2.1 Comments & Whitespace
- Whitespace (spaces, tabs, carriage returns, and newlines) serves as a token separator and is otherwise ignored.
- **Single-line comments**: Begin with `//` and extend to the end of the line.
- **Multi-line block comments**: Enclosed in `/* ... */`. Unterminated block comments trigger a compiler error.

### 2.2 Identifiers
Identifiers must start with an ASCII letter (`a`-`z`, `A`-`Z`) or underscore (`_`), followed by any number of letters, digits (`0`-`9`), or underscores. Identifiers are case-sensitive.

### 2.3 Reserved Keywords
The following 27 keywords are strictly reserved by the language and cannot be used as user-defined identifier names:

| Category | Keywords |
|---|---|
| Control Flow | `if`, `else`, `while`, `for`, `in`, `return` |
| Declarations | `let`, `fn`, `model`, `layer`, `input` |
| Primitive Types | `int`, `float`, `bool`, `string`, `void`, `tensor` |
| Layer & Activations | `Dense`, `relu`, `sigmoid`, `tanh`, `softmax`, `linear` |
| Tensor Built-ins | `zeros`, `ones`, `rand` |
| Boolean Literals | `true`, `false` |
| I/O Built-in | `print` |

### 2.4 Literals
- **Integer Literals (`IntLit`)**: Sequences of ASCII digits `[0-9]+` (e.g., `0`, `42`, `1000`). Mapped to signed 64-bit integers.
- **Floating-Point Literals (`FloatLit`)**: Contiguous digits followed by a period and fractional digits `[0-9]+\.[0-9]+` (e.g., `0.0`, `3.14`, `1.5`).
- **Boolean Literals**: The literal tokens `true` and `false`.
- **String Literals (`StringLit`)**: Sequences of characters enclosed in double quotes `"..."`. Supported escape sequences include `\\`, `\"`, `\n`, and `\t`.
- **Tensor Literals**: 1D bracketed vectors `[1.0, 2.0]` or 2D bracketed matrices `[[1.0, 2.0], [3.0, 4.0]]`.

### 2.5 Operators & Delimiters
- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Matrix Multiplication**: `@`
- **Comparison & Equality**: `<`, `<=`, `>`, `>=`, `==`, `!=`
- **Logical**: `&&`, `||`, `!`
- **Range Operator**: `..` (end-exclusive loop range)
- **Delimiters**: `(`, `)`, `{`, `}`, `[`, `]`, `,`, `:`, `;`, `->`, `.`

---

## 3. Type System

Exatron employs a strict, static nominal type system. Every variable declaration must include an explicit type annotation.

### 3.1 Type Mapping Table

| Exatron Type | Transpiled C++23 Representation | Description |
|---|---|---|
| `int` | `int64_t` | 64-bit signed integer |
| `float` | `float` | 32-bit single-precision IEEE 754 floating-point scalar |
| `bool` | `bool` | Boolean value (`true` or `false`) |
| `string` | `std::string` | UTF-8 dynamic character sequence |
| `void` | `void` | Absence of a return value (functions only) |
| `tensor<R, C>` | `exatron_rt::Tensor` | Statically shaped 2D floating-point matrix |
| `tensor<N>` | `exatron_rt::Tensor` | Statically shaped 1D row vector (normalized to `tensor<1, N>`) |
| `<ModelName>` | `struct ex_<ModelName>` | User-declared neural network model struct |

### 3.2 Tensor Shape Rules
- Tensors are restricted to rank 1 and rank 2 only.
- Specifying more than two dimensions (e.g., `tensor<1, 2, 3>`) results in a compile-time `ParseError`.
- Every dimension parameter must be an integer strictly greater than zero.
- A 1D tensor `tensor<N>` is automatically normalized to a 2D row vector `tensor<1, N>` by the parser and type checker.

### 3.3 No-Implicit-Conversion Policy
Exatron enforces strict type isolation:
- There is **no implicit integer-to-float promotion**. Writing `let x: float = 1;` or `1 + 2.0` produces a compile-time `TypeError`.
- Mixed-type comparisons (`x == 1.0` where `x` is `int`) are rejected.
- Narrowing conversions and string concatenations via `+` are not supported.

---

## 4. Declarations & Statements

### 4.1 Variable Declaration (`let`)
Variables are immutable or mutable bindings scoped to their enclosing block. The type annotation is mandatory.

```text
let identifier: Type = Expression;
```

**Example:**
```text
let learning_rate: float = 0.001;
let epochs: int = 50;
let is_training: bool = false;
```

### 4.2 Assignment
Assigns a new value to an already declared variable within scope. The right-hand side type must match the variable's declared type.

```text
identifier = Expression;
```

### 4.3 Conditional Branching (`if`, `else if`, `else`)
Conditional statements evaluate a boolean condition expression and execute branch blocks. An optional chained `else if` or terminal `else` block is supported:

```text
if (condition) {
    // then block
} else if (other_condition) {
    // else-if block
} else {
    // else block
}
```

### 4.4 While Loops (`while`)
Executes a body block repeatedly as long as the condition evaluates to `true`:

```text
while (condition) {
    // loop body
}
```

### 4.5 Range-Based For Loops (`for`)
Iterates over a contiguous range of integers. The upper bound is **end-exclusive** (`a..b` iterates from `a` up to `b - 1`). The loop bounds are evaluated exactly once prior to loop entry:

```text
for (var in start_expr..end_expr) {
    // loop body
}
```

**Example:**
```text
let sum: int = 0;
for (i in 0..5) {
    sum = sum + i; // iterates with i = 0, 1, 2, 3, 4
}
```

### 4.6 Return Statements (`return`)
Returns control from a function:
- Functions declaring a non-`void` return type must supply an expression matching that type: `return expr;`.
- Functions declaring `-> void` must use a bare return: `return;` (or implicitly return at the end of the block).
- Supplying an expression in a `void` function or omitting an expression in a non-`void` function produces a compile-time `TypeError`.

### 4.7 Print Statement (`print`)
Prints any scalar or tensor value to standard output, followed by a newline:

```text
print(expr);
```

---

## 5. Functions

### 5.1 Function Declarations
Functions are defined at the top level using the `fn` keyword. Functions accept zero or more comma-separated typed parameters and must explicitly state a return type:

```text
fn name(param1: Type1, param2: Type2) -> ReturnType {
    // function body
}
```

### 5.2 The Required Entry Point (`main`)
Every valid Exatron program must define an entry-point function with the exact signature:

```text
fn main() -> int {
    return 0;
}
```

Programs missing `main` or defining `main` with parameters or a non-integer return type fail semantic analysis.

### 5.3 Exhaustive Return Path Requirement
Non-`void` functions must guarantee a return statement on every executable path. A block is verified to return if its final statement is a `return` statement or an `if-else` statement where both branches guarantee a return. `while` and `for` loops do not satisfy return-path guarantees due to potential zero-iteration execution.

### 5.4 Forward Function References
Functions can be invoked before their textual declaration in the source file. The compiler automatically emits C++ forward prototypes for all declared functions prior to emitting their definitions.

---

## 6. Expressions & Precedence

The table below lists operator precedence in Exatron from highest to lowest:

| Precedence | Level | Operator(s) | Associativity | Description |
|---|---|---|---|---|
| 1 (Highest) | Primary & Postfix | `()`, `ident`, literals, `.method()`, `call()` | Left-to-right | Literals, function calls, method invocations |
| 2 | Unary | `-`, `!` | Right-to-left | Numerical negation, boolean NOT |
| 3 | Multiplicative | `*`, `/`, `%`, `@` | Left-to-right | Multiplication, division, modulo, matrix multiplication |
| 4 | Additive | `+`, `-` | Left-to-right | Numerical / tensor addition and subtraction |
| 5 | Relational | `<`, `<=`, `>`, `>=` | Left-to-right | Relational comparisons (scalars) |
| 6 | Equality | `==`, `!=` | Left-to-right | Value equality and inequality |
| 7 | Logical AND | `&&` | Left-to-right | Short-circuit boolean AND |
| 8 (Lowest) | Logical OR | `\|\|` | Left-to-right | Short-circuit boolean OR |

---

## 7. Tensors & Matrix Multiplication

### 7.1 Built-in Tensor Constructors
Tensors can be constructed using initialization functions that accept positive integer row and column counts:
- `zeros(R, C)`: Allocates an $R \times C$ matrix initialized to `0.0f`.
- `ones(R, C)`: Allocates an $R \times C$ matrix initialized to `1.0f`.
- `rand(R, C)`: Allocates an $R \times C$ matrix initialized with deterministic pseudorandom floating-point values in $[-0.5, 0.5)$.

### 7.2 Element-Wise Arithmetic
The binary operators `+`, `-`, and `*` perform element-wise arithmetic when applied to two tensors of identical shape:
$$\text{tensor}\langle R, C \rangle \times \text{tensor}\langle R, C \rangle \to \text{tensor}\langle R, C \rangle$$

### 7.3 Matrix Multiplication (`@`)
The `@` operator computes formal 2D linear algebraic matrix multiplication:
$$\text{tensor}\langle M, K \rangle \mathbin{@} \text{tensor}\langle K, N \rangle \to \text{tensor}\langle M, N \rangle$$

**Geometric Invariant Rule**:
The column count of the left operand must exactly match the row count of the right operand ($K_1 == K_2$). If dimensions mismatch, the type checker reports an error specifying both tensor geometries before native compilation begins.

---

## 8. Neural Networks with Exatron: A Neural Network Description Language

Exatron treats neural network architectures as first-class linguistic constructs. Rather than importing third-party libraries or imperatively managing memory buffers, developers declare neural network topologies directly using the `model` block. The Exatron compiler verifies dimension connectivity at compile time and emits standalone, zero-dependency C++23 evaluation code.

---

### 8.1 Overview and Current Scope

> [!IMPORTANT]
> **What Exatron's Neural Network Support IS:**
> - **A Feed-Forward Network Description Language**: Allows developers to specify multi-layer neural network architectures declaratively.
> - **Compile-Time Geometric Safety**: Automatically checks all tensor dimensions, matrix multiplication shapes, and layer connectivity before emitting code.
> - **Transpiled C++23 Forward Pass**: Emits self-contained, optimized C++ structs with an embedded linear-algebra runtime (`exatron_rt`).
> - **Deterministic Execution**: Generates fully reproducible weights across platforms via a deterministic Linear Congruential Generator (LCG).
>
> **What Exatron's Neural Network Support IS NOT (Yet):**
> - **No Training or Backpropagation**: Exatron does not currently synthesize backward passes, compute gradients, or implement automatic differentiation (autograd).
> - **No Loss Functions or Optimizers**: Functions such as MSE, cross-entropy, SGD, and Adam are not implemented in the language runtime.
> - **No File / Dataset I/O**: Exatron cannot read CSV, image, or binary dataset files from disk.
> - **No Weight Saving or Loading**: Weights cannot be loaded from or exported to disk checkpoints.
> - **Untrained Weights**: Output values are computed from deterministic pseudo-random initial weights and do not represent learned intelligence or meaningful predictions.

---

### 8.2 Machine Learning Concepts Refresher for Students

To understand Exatron's neural network abstractions, review these core concepts:

1. **Tensor**: A multidimensional array of floating-point numbers. In Exatron, tensors are rank 1 (row vectors of type `tensor<1, N>`) or rank 2 (matrices of type `tensor<Rows, Cols>`).
2. **Layer**: A structural transformation unit. A `Dense` (fully connected) layer maps an input feature vector to an output feature vector using learnable parameters.
3. **Weights ($W$) and Bias ($b$)**: 
   - **Weight Matrix ($W$)**: A matrix of coefficients scaling input signals.
   - **Bias Vector ($b$)**: A constant additive offset applied to shift activations independently of inputs.
4. **Activation Function**: A non-linear mathematical mapping applied element-wise to layer outputs, enabling the network to approximate complex non-linear functions.
5. **Forward Pass**: The sequential computation propagating an input tensor through successive layers to yield an output tensor.
6. **Probabilities**: Non-negative values summing to 1.0 produced by the `softmax` activation, representing categorical distribution likelihoods.

#### The Dense Layer Mathematical Formulation

For an input vector $x$ passing through a Dense layer:
$$y = \text{activation}(x @ W + b)$$

The Exatron runtime enforces strict dimensional agreement at every step of this formula:

| Quantity | Mathematical Symbol | Tensor Shape | Description |
|---|---|---|---|
| Input Vector | $x$ | `tensor<1, in_features>` | 1D row vector of input features |
| Weight Matrix | $W$ | `tensor<in_features, out_features>` | Linear transformation weights |
| Matrix Product | $x @ W$ | `tensor<1, out_features>` | Linear projection onto output space |
| Bias Vector | $b$ | `tensor<1, out_features>` | Additive bias term per output feature |
| Affine Sum | $x @ W + b$ | `tensor<1, out_features>` | Pre-activation linear step |
| Output Vector | $y$ | `tensor<1, out_features>` | Post-activation output features |

This formulation directly mirrors the C++ runtime implementation in `include/runtime_emitter.hpp`:
```cpp
Tensor linear_step(const Tensor& input) const {
    return add_bias(matmul(input, weights), bias);
}
```

---

### 8.3 Exatron as a Neural-Network Description Language

Traditional deep learning frameworks (such as PyTorch or TensorFlow) construct neural networks imperatively in Python: developer scripts instantiate objects, register hooks, and manually pass buffers through dynamic call graphs.

In contrast, Exatron treats models **declaratively**. A `model` block specifies an architectural contract:
- The input tensor geometry is explicitly declared.
- Layers are declared with their input/output dimensions and activation functions.
- The compiler automatically derives layer connectivity, verifies dimension invariants, and synthesizes the C++ struct and evaluation pass.

#### What the Compiler Generates for You

When compiling a `model` declaration, Exatron generates:
1. **Model Struct**: A C++ struct named `struct ex_<ModelName>`.
2. **Member Layers**: `exatron_rt::DenseLayer` member variables for each declared layer (`ex_fc1`, `ex_fc2`, etc.).
3. **Weight Initialization**: Initializes each layer's weight tensor via `Tensor::rand(in_features, out_features)` (values in $[-0.5, 0.5)$) and bias tensor via `Tensor::zeros(1, out_features)`.
4. **Synthesized `.forward()` Method**: A member function chaining the linear steps and activations:
   ```cpp
   exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
       exatron_rt::Tensor x = input;
       x = exatron_rt::relu(ex_fc1.linear_step(x));
       x = exatron_rt::softmax(ex_fc2.linear_step(x));
       return x;
   }
   ```

#### Exatron Source vs. Generated C++ Line Count Comparison

Because Exatron abstracts boilerplate memory management, struct declaration, and runtime setup, programs written in Exatron are significantly more compact than their generated standalone C++ counterparts. The following table records real line counts measured with `wc -l`:

| Example Program | Exatron Source Lines | Generated C++ Lines | Line Ratio (C++ / Exatron) |
|---|---|---|---|
| `01_linear_single_layer.ex` | 23 | 212 | 9.2x |
| `02_binary_classifier.ex` | 37 | 224 | 6.1x |
| `03_multiclass_iris_shape.ex` | 25 | 216 | 8.6x |
| `04_wide_vs_deep.ex` (2 models) | 37 | 233 | 6.3x |
| `05_hybrid_pipeline.ex` | 52 | 239 | 4.6x |

---

### 8.4 Tensors in Practice

Exatron provides built-in syntax and primitives for manipulating tensors:

#### Tensor Literals
- **1D Row Vector Literal**: `[1.0, 2.0, 3.0]` declares a vector of type `tensor<1, 3>`. Single-dimension bracket literals are automatically normalized to 2D row vectors.
- **2D Matrix Literal**: `[[1.0, 2.0], [3.0, 4.0]]` declares a matrix of type `tensor<2, 2>`.

#### Built-in Constructors
- `zeros(R, C)`: Allocates a $R \times C$ tensor populated entirely with `0.0`.
- `ones(R, C)`: Allocates a $R \times C$ tensor populated entirely with `1.0`.
- `rand(R, C)`: Allocates a $R \times C$ tensor populated with pseudo-random values in $[-0.5, 0.5)$ from the deterministic LCG.

#### Operators
- **Element-Wise Arithmetic (`+`, `-`, `*`)**: Operates on two tensors of identical shape (`tensor<R, C>`). Mismatched dimensions trigger a compile-time `TypeError`.
- **Matrix Multiplication (`@`)**: Multiplies two matrices (`tensor<M, K> @ tensor<K, N> -> tensor<M, N>`). The inner dimensions must match exactly ($K_1 == K_2$).

#### Verified Real Example
```text
fn main() -> int {
    let A: tensor<1, 3> = [[1.0, 2.0, 3.0]];
    let W: tensor<3, 2> = [[0.5, -0.5], [1.0, 0.0], [-1.0, 1.0]];
    let prod: tensor<1, 2> = A @ W;
    print(prod);
    return 0;
}
```
**Real Output:**
```text
[[-0.5000, 2.5000]]
```

---

### 8.5 Declaring a Model

A model declaration defines the architecture topology:

```text
model NetworkName {
    input: tensor<Rows, Cols>;
    layer layer_name_1: Dense(in_1, out_1, activation_1);
    layer layer_name_2: Dense(in_2, out_2, activation_2);
}
```

#### Declaration Rules
1. **Mandatory Input**: The first statement must be `input: tensor<Rows, Cols>;`. Currently, single-sample evaluation requires `Rows = 1`.
2. **At Least One Layer**: A model must define at least one `Dense` layer.
3. **Sequential Dimension Chaining**:
   - `layer_1.in_features` must equal `input.Cols`.
   - For every layer $i > 1$, `layer_i.in_features` must equal `layer_{i-1}.out_features`.
4. **Synthesized Return Type**: The return type of `net.forward(x)` is statically inferred as `tensor<input.Rows, last_layer.out_features>`.

#### Supported Activation Functions

| Activation | Mathematical Formula | Range | Standard Use Case |
|---|---|---|---|
| `relu` | $f(x) = \max(0, x)$ | $[0, \infty)$ | Hidden layers; mitigates vanishing gradient problem |
| `sigmoid` | $f(x) = \frac{1}{1 + e^{-x}}$ | $(0, 1)$ | Binary classification output; outputs scalar probability |
| `tanh` | $f(x) = \tanh(x) = \frac{e^x - e^{-x}}{e^x + e^{-x}}$ | $(-1, 1)$ | Hidden layers; zero-centered activation |
| `softmax` | $f(x_i) = \frac{e^{x_i - \max(x)}}{\sum_j e^{x_j - \max(x)}}$ | $(0, 1)$, $\sum_j = 1$ | Multi-class classification output; categorical distribution |
| `linear` | $f(x) = x$ | $(-\infty, \infty)$ | Regression output; identity pass-through without saturation |

#### Parameter-Count Formula

Each `Dense(in_features, out_features, activation)` layer contains a weight matrix and a bias vector:
$$\text{Weights} = \text{in} \times \text{out}$$
$$\text{Biases} = 1 \times \text{out} = \text{out}$$
$$\text{Total Parameters per Layer} = (\text{in} \times \text{out}) + \text{out}$$

#### Worked Parameter Count Table

| Model Architecture | Layer Breakdown | Calculation | Total Parameters |
|---|---|---|---|
| Single Layer (3 $\to$ 1) | `out: Dense(3, 1)` | $(3 \times 1) + 1$ | 4 |
| Binary MLP (2 $\to$ 4 $\to$ 1) | `hidden: Dense(2, 4)`<br>`out: Dense(4, 1)` | $(2 \times 4 + 4) = 12$<br>$(4 \times 1 + 1) = 5$ | 17 |
| Iris Classifier (4 $\to$ 8 $\to$ 4 $\to$ 3) | `fc1: Dense(4, 8)`<br>`fc2: Dense(8, 4)`<br>`fc3: Dense(4, 3)` | $(4 \times 8 + 8) = 40$<br>$(8 \times 4 + 4) = 36$<br>$(4 \times 3 + 3) = 15$ | 91 |
| Wide Network (4 $\to$ 16 $\to$ 2) | `hidden: Dense(4, 16)`<br>`out: Dense(16, 2)` | $(4 \times 16 + 16) = 80$<br>$(16 \times 2 + 2) = 34$ | 114 |
| Deep Network (4 $\to$ 6 $\to$ 6 $\to$ 6 $\to$ 2) | `h1: Dense(4, 6)`<br>`h2: Dense(6, 6)`<br>`h3: Dense(6, 6)`<br>`out: Dense(6, 2)` | $(4 \times 6 + 6) = 30$<br>$(6 \times 6 + 6) = 42$<br>$(6 \times 6 + 6) = 42$<br>$(6 \times 2 + 2) = 14$ | 128 |

---

### 8.6 Running a Model

#### Instantiation & Evaluation
A model is instantiated using its generated constructor and evaluated via `.forward()`:

```text
let net: IrisClassifier = IrisClassifier();
let sample: tensor<1, 4> = [[5.1, 3.5, 1.4, 0.2]];
let probs: tensor<1, 3> = net.forward(sample);
print(probs);
```

#### Mathematical Sanity Properties
When evaluating networks, Exatron outputs satisfy mathematical invariants:
1. **Softmax Normalization**: All elements are strictly non-negative and sum to $1.0$ within floating-point precision ($|\sum p_i - 1.0| < 10^{-4}$).
2. **Sigmoid Boundedness**: All output activations fall strictly within the open interval $(0.0, 1.0)$.
3. **Tanh Boundedness**: All output activations fall strictly within the open interval $(-1.0, 1.0)$.
4. **ReLU Non-Negativity**: For every element $v$, $v \ge 0.0$.

#### Deterministic RNG & Manual Cross-Checking

Exatron's runtime initializes weights using a shared global Linear Congruential Generator (LCG) seeded at `1337`:
$$S_{k+1} = (S_k \times 1664525 + 1013904223) \bmod 2^{32}$$
$$\text{value} = \frac{S_{k+1} \gg 8}{16777216.0} - 0.5$$

Because this state is global and deterministic:
1. **Identical Executions**: Two runs of the same compiled binary produce bit-for-bit identical outputs.
2. **Predictable Sequence**: Instantiating a model consumes the exact pseudo-random sequence corresponding to its weight dimensions. Any user `rand()` calls executed subsequently advance this same state.
3. **Manual Cross-Checking**: Because the sequence is known and deterministic, developers can manually compute expected weights in external tools (e.g. Python) to verify numerical execution against Exatron's C++ output.

---

### 8.7 Compile-Time Shape Safety

Exatron catches geometric incompatibilities during semantic analysis before native compilation begins.

#### Error Catalog

#### 1. Layer Chain Dimension Mismatch
Triggered when layer $i$ expects an input dimension different from layer $i-1$'s output dimension.

**Minimal Trigger (`tests/errors/err_layer_chain.ex`):**
```text
model BadChain {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(6, 2, softmax);
}
fn main() -> int { return 0; }
```
**Exact Diagnostic:**
```text
[Type Error] Line 4, Col 5: Layer 'fc2' expects input dimension 6, but previous dimension is 8
```
**Fix:** Match `fc2`'s input dimension to `fc1`'s output dimension: `layer fc2: Dense(8, 2, softmax);`.

---

#### 2. Matrix Multiplication (`@`) Dimension Mismatch
Triggered when the column count of the left operand does not equal the row count of the right operand.

**Minimal Trigger (`tests/errors/err_matmul_dim.ex`):**
```text
fn main() -> int {
    let a: tensor<2, 3> = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]];
    let b: tensor<2, 2> = [[1.0, 2.0], [3.0, 4.0]];
    let c: tensor<2, 2> = a @ b;
    return 0;
}
```
**Exact Diagnostic:**
```text
[Type Error] Line 4, Col 27: Matrix multiplication dimension mismatch: cannot multiply tensor<2, 3> by tensor<2, 2>
```
**Fix:** Adjust dimensions so inner dimensions match ($3 == 3$), e.g. `b: tensor<3, 2>`.

---

#### 3. Wrong Forward Input Shape
Triggered when the tensor passed to `.forward()` does not match the model's declared input shape.

**Minimal Trigger:**
```text
model Net {
    input: tensor<1, 3>;
    layer l1: Dense(3, 2, relu);
}
fn main() -> int {
    let m: Net = Net();
    let x: tensor<1, 4> = [[1.0, 2.0, 3.0, 4.0]];
    let y: tensor<1, 2> = m.forward(x);
    return 0;
}
```
**Exact Diagnostic:**
```text
[Type Error] Line 8, Col 35: forward() expects input of type tensor<1, 3> but got tensor<1, 4>
```
**Fix:** Supply an input tensor matching the declared shape: `let x: tensor<1, 3> = [[1.0, 2.0, 3.0]];`.

---

#### 4. Jagged Tensor Literal
Triggered when rows of a 2D tensor literal have inconsistent column lengths.

**Minimal Trigger:**
```text
fn main() -> int {
    let t: tensor<2, 2> = [[1.0, 2.0], [3.0]];
    return 0;
}
```
**Exact Diagnostic:**
```text
[Type Error] Line 2, Col 38: Jagged tensor literal: expected 2 columns but got 1
```
**Fix:** Ensure every row contains exactly 2 elements: `[[1.0, 2.0], [3.0, 4.0]]`.

---

#### 5. Unknown Model Type
Triggered when attempting to instantiate or annotate a model type that has not been declared.

**Minimal Trigger:**
```text
fn main() -> int {
    let m: FooNet = FooNet();
    return 0;
}
```
**Exact Diagnostic:**
```text
[Type Error] Line 2, Col 5: Unknown type 'FooNet'
```
**Fix:** Declare `model FooNet { ... }` before using it.

---

### 8.8 Five Worked Examples

The following 5 examples are verified against the compiler and included in the automated test suite under `docs/examples/nn/`.

---

#### Example 1: Linear Single Layer (`docs/examples/nn/01_linear_single_layer.ex`)

**Goal:** Implements a single Dense layer with linear activation, mapping a 3-element feature vector to a 1-element scalar output ($y = x @ W + b$).

**Architecture Diagram:**
```text
Input (1x3) ---> [Dense(3, 1, linear)] ---> Output (1x1)
```

**Complete Source:**
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

**Parameter-Count Table:**
| Layer | Input Size | Output Size | Weights ($in \times out$) | Biases ($out$) | Total Parameters |
|---|---|---|---|---|---|
| `out` | 3 | 1 | $3 \times 1 = 3$ | $1$ | 4 |
| **Total** | | | **3** | **1** | **4** |

**Compile and Run Command:**
```bash
exatron docs/examples/nn/01_linear_single_layer.ex -o build/nn_01_linear_single_layer --run
```

**Real Output:**
```text
Linear Regressor: 3 inputs -> 1 output
Input:
[[1.0000, 2.0000, 3.0000]]
Output (untrained, random weights):
[[-0.3233]]
```

**Line-by-Line Explanation:**
- Lines 5–8: Declares `model LinearRegressor` expecting input of shape `tensor<1, 3>` with a single layer named `out` transforming 3 inputs to 1 output via `linear` activation.
- Line 11: Instantiates `LinearRegressor` as `net`, initializing its weights deterministically.
- Line 12: Defines input feature vector `x` with values `[1.0, 2.0, 3.0]`.
- Line 18: Calls `net.forward(x)`, performing matrix multiplication and bias addition.
- Line 20: Prints the resulting output scalar `[[-0.3233]]`.

**Generated C++ Excerpt:**
```cpp
struct ex_LinearRegressor {
    exatron_rt::DenseLayer ex_out{3, 1};

    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
        exatron_rt::Tensor x = input;
        x = exatron_rt::linear(ex_out.linear_step(x));
        return x;
    }
};
```

**Note on Reading Output:**
The output value `[[-0.3233]]` is produced from untrained pseudo-random weights initialized in $[-0.5, 0.5)$ and does not represent a fitted linear regression line.

---

#### Example 2: Binary Classifier (`docs/examples/nn/02_binary_classifier.ex`)

**Goal:** Implements a binary classification architecture with a 2-input feature space, a 4-unit ReLU hidden layer, and a single sigmoid output unit yielding an activation in $(0, 1)$.

**Architecture Diagram:**
```text
Input (1x2) ---> [Dense(2, 4, relu)] ---> [Dense(4, 1, sigmoid)] ---> Output (1x1)
```

**Complete Source:**
```text
// 02_binary_classifier.ex
// Binary classifier: 2 -> 4 (relu) -> 1 (sigmoid).
// Sigmoid output is in (0, 1), interpretable as a probability.

model BinaryClassifier {
    input: tensor<1, 2>;
    layer hidden: Dense(2, 4, relu);
    layer out: Dense(4, 1, sigmoid);
}

fn main() -> int {
    let net: BinaryClassifier = BinaryClassifier();

    print("Binary Classifier: 2 -> 4 (relu) -> 1 (sigmoid)");

    let x1: tensor<1, 2> = [[0.0, 0.0]];
    let y1: tensor<1, 1> = net.forward(x1);
    print("Input [0, 0] -> output:");
    print(y1);

    let x2: tensor<1, 2> = [[1.0, 0.0]];
    let y2: tensor<1, 1> = net.forward(x2);
    print("Input [1, 0] -> output:");
    print(y2);

    let x3: tensor<1, 2> = [[0.0, 1.0]];
    let y3: tensor<1, 1> = net.forward(x3);
    print("Input [0, 1] -> output:");
    print(y3);

    let x4: tensor<1, 2> = [[1.0, 1.0]];
    let y4: tensor<1, 1> = net.forward(x4);
    print("Input [1, 1] -> output:");
    print(y4);

    return 0;
}
```

**Parameter-Count Table:**
| Layer | Input Size | Output Size | Weights ($in \times out$) | Biases ($out$) | Total Parameters |
|---|---|---|---|---|---|
| `hidden` | 2 | 4 | $2 \times 4 = 8$ | $4$ | 12 |
| `out` | 4 | 1 | $4 \times 1 = 4$ | $1$ | 5 |
| **Total** | | | **12** | **5** | **17** |

**Compile and Run Command:**
```bash
exatron docs/examples/nn/02_binary_classifier.ex -o build/nn_02_binary_classifier --run
```

**Real Output:**
```text
Binary Classifier: 2 -> 4 (relu) -> 1 (sigmoid)
Input [0, 0] -> output:
[[0.5000]]
Input [1, 0] -> output:
[[0.5088]]
Input [0, 1] -> output:
[[0.4959]]
Input [1, 1] -> output:
[[0.5081]]
```

**Line-by-Line Explanation:**
- Lines 5–9: Declares `BinaryClassifier` with a 2-input vector, a 4-neuron hidden layer using `relu`, and a 1-neuron output layer using `sigmoid`.
- Line 12: Instantiates the network.
- Lines 16–33: Evaluates 4 distinct inputs representing the corners of a unit square ($[0,0], [1,0], [0,1], [1,1]$), printing the sigmoid activation for each.

**Generated C++ Excerpt:**
```cpp
struct ex_BinaryClassifier {
    exatron_rt::DenseLayer ex_hidden{2, 4};
    exatron_rt::DenseLayer ex_out{4, 1};

    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
        exatron_rt::Tensor x = input;
        x = exatron_rt::relu(ex_hidden.linear_step(x));
        x = exatron_rt::sigmoid(ex_out.linear_step(x));
        return x;
    }
};
```

**Note on Reading Output:**
Notice that input `[0, 0]` produces exactly `[[0.5000]]`. Because $x = [0, 0]$ yields $x @ W = [0,0,0,0]$, and bias starts at zero, the hidden ReLU outputs zero, passing zero into the sigmoid: $\text{sigmoid}(0) = 0.5000$. Outputs for other samples fall in $(0, 1)$. Because the network is untrained, these values reflect pseudo-random weights rather than a trained decision boundary.

---

#### Example 3: Multi-Class Iris Classifier (`docs/examples/nn/03_multiclass_iris_shape.ex`)

**Goal:** Models a 3-layer architecture matching the classic Iris dataset feature and class dimensions (4 features $\to$ 3 target classes) using ReLU, Tanh, and Softmax activations.

**Architecture Diagram:**
```text
Input (1x4) ---> [Dense(4, 8, relu)] ---> [Dense(8, 4, tanh)] ---> [Dense(4, 3, softmax)] ---> Output (1x3)
```

**Complete Source:**
```text
// 03_multiclass_iris_shape.ex
// Iris-shaped multi-class classifier: 4 -> 8 (relu) -> 4 (tanh) -> 3 (softmax).
// Softmax output rows sum to 1.0, representing class probabilities.

model IrisClassifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 4, tanh);
    layer fc3: Dense(4, 3, softmax);
}

fn main() -> int {
    let net: IrisClassifier = IrisClassifier();
    let features: tensor<1, 4> = [[5.1, 3.5, 1.4, 0.2]];

    print("Iris Classifier: 4 -> 8 (relu) -> 4 (tanh) -> 3 (softmax)");
    print("Input features:");
    print(features);

    let probs: tensor<1, 3> = net.forward(features);
    print("Output probabilities (softmax, sum to 1.0):");
    print(probs);

    return 0;
}
```

**Parameter-Count Table:**
| Layer | Input Size | Output Size | Weights ($in \times out$) | Biases ($out$) | Total Parameters |
|---|---|---|---|---|---|
| `fc1` | 4 | 8 | $4 \times 8 = 32$ | $8$ | 40 |
| `fc2` | 8 | 4 | $8 \times 4 = 32$ | $4$ | 36 |
| `fc3` | 4 | 3 | $4 \times 3 = 12$ | $3$ | 15 |
| **Total** | | | **76** | **15** | **91** |

**Compile and Run Command:**
```bash
exatron docs/examples/nn/03_multiclass_iris_shape.ex -o build/nn_03_multiclass_iris_shape --run
```

**Real Output:**
```text
Iris Classifier: 4 -> 8 (relu) -> 4 (tanh) -> 3 (softmax)
Input features:
[[5.1000, 3.5000, 1.4000, 0.2000]]
Output probabilities (softmax, sum to 1.0):
[[0.3289, 0.3968, 0.2743]]
```

**Line-by-Line Explanation:**
- Lines 5–10: Declares `IrisClassifier` with 4 inputs and 3 dense layers (`fc1`, `fc2`, `fc3`), terminating with `softmax`.
- Line 13: Instantiates the network.
- Line 14: Declares a 4-element feature vector corresponding to sepal and petal measurements (`[5.1, 3.5, 1.4, 0.2]`).
- Line 20: Evaluates the network via `.forward()`.
- Line 22: Prints the 3 predicted class probabilities.

**Generated C++ Excerpt:**
```cpp
struct ex_IrisClassifier {
    exatron_rt::DenseLayer ex_fc1{4, 8};
    exatron_rt::DenseLayer ex_fc2{8, 4};
    exatron_rt::DenseLayer ex_fc3{4, 3};

    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
        exatron_rt::Tensor x = input;
        x = exatron_rt::relu(ex_fc1.linear_step(x));
        x = exatron_rt::tanh_act(ex_fc2.linear_step(x));
        x = exatron_rt::softmax(ex_fc3.linear_step(x));
        return x;
    }
};
```

**Note on Reading Output:**
Verify the softmax normalization property:
$$0.3289 + 0.3968 + 0.2743 = 1.0000$$
The output values form a valid probability distribution across 3 classes. Because weights are untrained, the highest probability (`0.3968` for class 2) is a random artifact rather than a true Iris species classification.

---

#### Example 4: Wide vs. Deep Architectures (`docs/examples/nn/04_wide_vs_deep.ex`)

**Goal:** Compares two distinct network topologies sharing identical input (4) and output (2) dimensions: a "wide" single-hidden-layer network (16 neurons) versus a "deep" three-hidden-layer network (6 neurons each), evaluating both on the same input vector.

**Architecture Diagram:**
```text
WideNet:
Input (1x4) ---> [Dense(4, 16, relu)] -----------------------------------------> [Dense(16, 2, softmax)] ---> Output (1x2)

DeepNet:
Input (1x4) ---> [Dense(4, 6, relu)] ---> [Dense(6, 6, relu)] ---> [Dense(6, 6, relu)] ---> [Dense(6, 2, softmax)] ---> Output (1x2)
```

**Complete Source:**
```text
// 04_wide_vs_deep.ex
// Two models with matching input (1x4) and output (1x2) shapes:
//   WideNet:  one wide hidden layer  (4 -> 16 -> 2)
//   DeepNet:  three narrow hidden layers (4 -> 6 -> 6 -> 6 -> 2)
// Both run on the same input. Parameter counts differ.

model WideNet {
    input: tensor<1, 4>;
    layer hidden: Dense(4, 16, relu);
    layer out: Dense(16, 2, softmax);
}

model DeepNet {
    input: tensor<1, 4>;
    layer h1: Dense(4, 6, relu);
    layer h2: Dense(6, 6, relu);
    layer h3: Dense(6, 6, relu);
    layer out: Dense(6, 2, softmax);
}

fn main() -> int {
    let wide: WideNet = WideNet();
    let deep: DeepNet = DeepNet();
    let x: tensor<1, 4> = [[1.0, 2.0, 3.0, 4.0]];

    print("=== Wide Network: 4 -> 16 -> 2 ===");
    print("Parameters: (4*16+16) + (16*2+2) = 114");
    let yw: tensor<1, 2> = wide.forward(x);
    print(yw);

    print("=== Deep Network: 4 -> 6 -> 6 -> 6 -> 2 ===");
    print("Parameters: (4*6+6) + (6*6+6) + (6*6+6) + (6*2+2) = 128");
    let yd: tensor<1, 2> = deep.forward(x);
    print(yd);

    return 0;
}
```

**Parameter-Count Table:**
| Model | Layer | Dimensions | Parameter Formula | Layer Params | Model Total |
|---|---|---|---|---|---|
| **WideNet** | `hidden`<br>`out` | $4 \to 16$<br>$16 \to 2$ | $(4 \times 16) + 16$<br>$(16 \times 2) + 2$ | 80<br>34 | **114** |
| **DeepNet** | `h1`<br>`h2`<br>`h3`<br>`out` | $4 \to 6$<br>$6 \to 6$<br>$6 \to 6$<br>$6 \to 2$ | $(4 \times 6) + 6$<br>$(6 \times 6) + 6$<br>$(6 \times 6) + 6$<br>$(6 \times 2) + 2$ | 30<br>42<br>42<br>14 | **128** |

**Compile and Run Command:**
```bash
exatron docs/examples/nn/04_wide_vs_deep.ex -o build/nn_04_wide_vs_deep --run
```

**Real Output:**
```text
=== Wide Network: 4 -> 16 -> 2 ===
Parameters: (4*16+16) + (16*2+2) = 114
[[0.6638, 0.3362]]
=== Deep Network: 4 -> 6 -> 6 -> 6 -> 2 ===
Parameters: (4*6+6) + (6*6+6) + (6*6+6) + (6*2+2) = 128
[[0.4411, 0.5589]]
```

**Line-by-Line Explanation:**
- Lines 6–10: Declares `WideNet` using a single 16-neuron hidden layer.
- Lines 12–18: Declares `DeepNet` using three cascaded 6-neuron hidden layers.
- Lines 21–22: Instantiates both models within the same program.
- Lines 26–28: Evaluates `wide` on input `x`, producing `[[0.6638, 0.3362]]`.
- Lines 32–34: Evaluates `deep` on the exact same input `x`, producing `[[0.4411, 0.5589]]`.

**Generated C++ Excerpt:**
```cpp
struct ex_WideNet {
    exatron_rt::DenseLayer ex_hidden{4, 16};
    exatron_rt::DenseLayer ex_out{16, 2};
    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const { ... }
};
struct ex_DeepNet {
    exatron_rt::DenseLayer ex_h1{4, 6};
    exatron_rt::DenseLayer ex_h2{6, 6};
    exatron_rt::DenseLayer ex_h3{6, 6};
    exatron_rt::DenseLayer ex_out{6, 2};
    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const { ... }
};
```

**Note on Reading Output:**
Both outputs sum to $1.0$ ($0.6638 + 0.3362 = 1.0$ and $0.4411 + 0.5589 = 1.0$). The deep architecture contains more parameters (128 vs 114) despite having narrower layers due to the repeated $6 \times 6$ transformations. Neither prediction has practical semantic significance without training.

---

#### Example 5: Hybrid Pipeline (`docs/examples/nn/05_hybrid_pipeline.ex`)

**Goal:** Demonstrates integrating neural network models with general-purpose programming constructs: defining sample-generation helper functions, wrapping `.forward()` inside a function accepting a model parameter, and executing forward passes iteratively inside a range loop.

**Architecture Diagram:**
```text
Helper Functions ---> [Classifier: Dense(3, 4, relu) -> Dense(4, 2, softmax)] ---> Loop Iteration
```

**Complete Source:**
```text
// 05_hybrid_pipeline.ex
// Mixes general-purpose features with a neural network model:
// helper functions, a wrapper around forward(), a loop over samples,
// and labeled printing.

model Classifier {
    input: tensor<1, 3>;
    layer fc1: Dense(3, 4, relu);
    layer fc2: Dense(4, 2, softmax);
}

fn make_sample(a: float, b: float, c: float) -> tensor<1, 3> {
    let t: tensor<1, 3> = [[a, b, c]];
    return t;
}

fn classify(net: Classifier, x: tensor<1, 3>) -> tensor<1, 2> {
    let result: tensor<1, 2> = net.forward(x);
    return result;
}

fn main() -> int {
    let net: Classifier = Classifier();

    print("=== Hybrid Pipeline: classify 4 samples ===");

    let s1: tensor<1, 3> = make_sample(1.0, 0.0, 0.0);
    let s2: tensor<1, 3> = make_sample(0.0, 1.0, 0.0);
    let s3: tensor<1, 3> = make_sample(0.0, 0.0, 1.0);
    let s4: tensor<1, 3> = make_sample(1.0, 1.0, 1.0);

    print("Sample 1: [1, 0, 0]");
    print(classify(net, s1));

    print("Sample 2: [0, 1, 0]");
    print(classify(net, s2));

    print("Sample 3: [0, 0, 1]");
    print(classify(net, s3));

    print("Sample 4: [1, 1, 1]");
    print(classify(net, s4));

    print("=== Running forward in a loop ===");
    let acc: tensor<1, 3> = [[0.5, 0.5, 0.5]];
    for (i in 0..3) {
        let out: tensor<1, 2> = net.forward(acc);
        print(out);
    }

    return 0;
}
```

**Parameter-Count Table:**
| Layer | Input Size | Output Size | Weights ($in \times out$) | Biases ($out$) | Total Parameters |
|---|---|---|---|---|---|
| `fc1` | 3 | 4 | $3 \times 4 = 12$ | $4$ | 16 |
| `fc2` | 4 | 2 | $4 \times 2 = 8$ | $2$ | 10 |
| **Total** | | | **20** | **6** | **26** |

**Compile and Run Command:**
```bash
exatron docs/examples/nn/05_hybrid_pipeline.ex -o build/nn_05_hybrid_pipeline --run
```

**Real Output:**
```text
=== Hybrid Pipeline: classify 4 samples ===
Sample 1: [1, 0, 0]
[[0.5085, 0.4915]]
Sample 2: [0, 1, 0]
[[0.4973, 0.5027]]
Sample 3: [0, 0, 1]
[[0.5085, 0.4915]]
Sample 4: [1, 1, 1]
[[0.5193, 0.4807]]
=== Running forward in a loop ===
[[0.5096, 0.4904]]
[[0.5096, 0.4904]]
[[0.5096, 0.4904]]
```

**Line-by-Line Explanation:**
- Lines 12–15: Function `make_sample` constructs a `tensor<1, 3>` from scalar float arguments, demonstrating functions returning tensor values.
- Lines 17–20: Function `classify` accepts a model instance `net: Classifier` and input `x`, demonstrating models passed as typed function arguments.
- Lines 27–30: Constructs 4 distinct test tensors using `make_sample`.
- Lines 32–42: Evaluates and prints predictions for all 4 samples through `classify()`.
- Lines 45–49: Executes `net.forward(acc)` iteratively across 3 iterations of a `for` loop, demonstrating repeated model evaluation in loop contexts.

**Generated C++ Excerpt:**
```cpp
exatron_rt::Tensor ex_make_sample(float ex_a, float ex_b, float ex_c) {
    exatron_rt::Tensor ex_t = exatron_rt::Tensor(1, 3, {ex_a, ex_b, ex_c});
    return ex_t;
}
exatron_rt::Tensor ex_classify(ex_Classifier ex_net, exatron_rt::Tensor ex_x) {
    exatron_rt::Tensor ex_result = ex_net.forward(ex_x);
    return ex_result;
}
```

**Note on Reading Output:**
All four sample classifications output valid probability vectors summing to 1.0. Inside the loop, because `acc` is unchanged, each iteration produces identical probabilities (`[[0.5096, 0.4904]]`), demonstrating deterministic idempotency.

---

### 8.9 Inspecting Generated C++

Using `--emit-cpp`, developers can inspect the transpiled code. Below is the mapping between Exatron source code and emitted C++ for `IrisClassifier`:

#### Exatron Source
```text
model IrisClassifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 4, tanh);
    layer fc3: Dense(4, 3, softmax);
}
```

#### Emitted C++23 Struct
```cpp
struct ex_IrisClassifier {
    exatron_rt::DenseLayer ex_fc1{4, 8};
    exatron_rt::DenseLayer ex_fc2{8, 4};
    exatron_rt::DenseLayer ex_fc3{4, 3};

    exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
        exatron_rt::Tensor x = input;
        x = exatron_rt::relu(ex_fc1.linear_step(x));
        x = exatron_rt::tanh_act(ex_fc2.linear_step(x));
        x = exatron_rt::softmax(ex_fc3.linear_step(x));
        return x;
    }
};
```

#### The `exatron_rt` Runtime Primitives
1. **`DenseLayer` Struct**:
   ```cpp
   struct DenseLayer {
       Tensor weights;
       Tensor bias;
       DenseLayer(int in_f, int out_f)
           : weights(Tensor::rand(in_f, out_f)), bias(Tensor::zeros(1, out_f)) {}
       Tensor linear_step(const Tensor& input) const {
           return add_bias(matmul(input, weights), bias);
       }
   };
   ```
2. **`matmul` Implementation**: Employs cache-friendly row-major 3-loop matrix multiplication:
   ```cpp
   inline Tensor matmul(const Tensor& a, const Tensor& b) {
       Tensor out(a.rows, b.cols);
       for (int i = 0; i < a.rows; ++i)
           for (int k = 0; k < a.cols; ++k) {
               float r = a.at(i, k);
               for (int j = 0; j < b.cols; ++j)
                   out.at(i, j) += r * b.at(k, j);
           }
       return out;
   }
   ```
3. **Activation Functions**: Implemented in clean, vectorized loops:
   - `relu`: `std::max(0.0f, v)`
   - `sigmoid`: `1.0f / (1.0f + std::exp(-v))`
   - `tanh_act`: `std::tanh(v)`
   - `softmax`: Numerically stabilized using `max_val` subtraction:
     ```cpp
     inline Tensor softmax(const Tensor& x) {
         Tensor out = x;
         for (int i = 0; i < out.rows; ++i) {
             float max_val = out.at(i, 0);
             for (int j = 1; j < out.cols; ++j)
                 max_val = std::max(max_val, out.at(i, j));
             float sum = 0.0f;
             for (int j = 0; j < out.cols; ++j) {
                 out.at(i, j) = std::exp(out.at(i, j) - max_val);
                 sum += out.at(i, j);
             }
             for (int j = 0; j < out.cols; ++j)
                 out.at(i, j) /= sum;
         }
         return out;
     }
     ```

---

### 8.10 What You Can Evaluate Today

Exatron allows developers to verify several critical properties of neural network systems:

1. **Shape Validation**: Guarantee at compile time that all matrix multiplications, layer cascades, and feature inputs are geometrically valid.
2. **Output Sanity Checks**: Confirm that probabilities normalize to 1.0, that bounded activations remain in range, and that linear outputs reflect matrix affine calculations.
3. **Determinism Verification**: Confirm that two independent program executions generate bit-for-bit identical outputs:
   ```bash
   ./build/nn_03_multiclass_iris_shape && ./build/nn_03_multiclass_iris_shape
   ```
   Both print: `[[0.3289, 0.3968, 0.2743]]`.
4. **Execution Performance & Benchmarking**: Evaluate raw CPU forward-pass latency using `time`:
   ```bash
   time ./build/nn_03_multiclass_iris_shape
   ```
   **Real Output:**
   ```text
   real    0m0.002s
   user    0m0.001s
   sys     0m0.001s
   ```
   *Execution finishes in ~2 milliseconds with zero Python runtime overhead.*

> [!WARNING]
> **Limitations on Learning & Evaluation**: Because backpropagation, dataset loading, and loss functions are not yet implemented, you cannot evaluate classification accuracy, cross-entropy loss, or training convergence. Weights are untrained, and outputs do not constitute trained predictions.

---

### 8.11 Regression-Testing Models with the Test Runner

The project test runner (`tests/run_tests.sh`) automates regression verification by compiling and executing programs against golden reference files (`tests/expected/*.out`).

#### Adding a Neural Network Regression Test
1. Save your model program into `docs/examples/nn/<name>.ex`.
2. Generate its golden reference output:
   ```bash
   exatron docs/examples/nn/<name>.ex -o build/<name> --run > tests/expected/<name>.out
   ```
3. Register the test in `tests/run_tests.sh` under `NN_TESTS`:
   ```bash
   NN_TESTS=(
       ...
       "<name>:docs/examples/nn/<name>.ex"
   )
   ```
4. Run `bash tests/run_tests.sh`. The test runner verifies clean zero-exit status and bitwise output matching.

---

### 8.12 Capability Matrix

The following table summarizes all 42 capabilities audited against Exatron v0.1.0:

| Category | Capability | Status | Diagnostic / Workaround |
|---|---|---|---|
| **Model Topology** | 1 Dense layer | SUPPORTED | Standard syntax |
| | N Dense layers (2..N) | SUPPORTED | Verified with up to 5 layers |
| | Activation: `relu` | SUPPORTED | Hidden non-linearity |
| | Activation: `sigmoid` | SUPPORTED | Binary classification output in $(0, 1)$ |
| | Activation: `tanh` | SUPPORTED | Zero-centered activation in $(-1, 1)$ |
| | Activation: `softmax` | SUPPORTED | Categorical distribution summing to $1.0$ |
| | Activation: `linear` | SUPPORTED | Identity regression pass-through |
| | Multiple models per file | SUPPORTED | Independent models compile cleanly |
| **Execution** | `forward()` correct shape | SUPPORTED | Synthesized const member function |
| | `forward()` wrong shape | NOT SUPPORTED | `[Type Error] forward() expects input of type ...` |
| | Mini-batches ($M > 1$) | NOT SUPPORTED | `[Type Error] forward() expects input of type tensor<1, N>`. (Workaround: declare `input: tensor<M, N>` explicitly) |
| | Multiple forward calls | SUPPORTED | Can evaluate multiple inputs sequentially |
| | Forward calls in loops | SUPPORTED | Can iterate over samples in `for` loops |
| | Passing model to function | SUPPORTED | Models pass as typed parameters |
| **Tensor Primitives** | 1D tensor literals | SUPPORTED | Normalized to `tensor<1, N>` |
| | 2D tensor literals | SUPPORTED | `tensor<R, C>` |
| | `zeros(R, C)` | SUPPORTED | Zero initialization |
| | `ones(R, C)` | SUPPORTED | Ones initialization |
| | `rand(R, C)` | SUPPORTED | Deterministic LCG in $[-0.5, 0.5)$ |
| **Tensor Math** | Element-wise `+`, `-`, `*` | SUPPORTED | Same dimensions required |
| | Matrix multiplication `@` | SUPPORTED | Checked at compile time ($K_1 == K_2$) |
| | Unary minus `-tensor` | NOT SUPPORTED | `[Type Error] Operator '-' cannot be applied to tensor` (Workaround: `zeros(R, C) - t`) |
| | Scalar-tensor operations | NOT SUPPORTED | `[Type Error] Invalid operand types for arithmetic` |
| | Element-wise division `/` | NOT SUPPORTED | `[Type Error] Invalid operand types for division` |
| | Tensor equality (`==`) | NOT SUPPORTED | `[Type Error] Equality comparison requires matching scalar types` |
| **Advanced Tensor** | Element indexing (`t[i]`) | NOT SUPPORTED | Parse error (brackets not supported on identifiers) |
| | Tensor slicing (`t[0..2]`) | NOT SUPPORTED | Parse error |
| | `sum()`, `mean()` | NOT SUPPORTED | `[Type Error] Undefined function` |
| | `max()`, `argmax()` | NOT SUPPORTED | `[Type Error] Undefined function` |
| | `reshape()`, `transpose()` | NOT SUPPORTED | `[Type Error] Undefined function` |
| **Learning & Weights** | Accessing layer weights | NOT SUPPORTED | Parse error (only `.forward()` method permitted) |
| | Setting layer weights | NOT SUPPORTED | No assignment syntax for layers |
| | Saving/loading weights | NOT SUPPORTED | No disk checkpointing mechanism |
| | Training / Backpropagation | NOT SUPPORTED | `[Type Error] Model has no method 'train'` |
| | Loss functions (`mse`) | NOT SUPPORTED | `[Type Error] Undefined function 'mse'` |
| | Optimizers (SGD, Adam) | NOT SUPPORTED | No optimizer construct |
| | Evaluation metrics | NOT SUPPORTED | No metrics API |
| | Dataset file ingestion | NOT SUPPORTED | `[Type Error] Undefined function 'read_file'` |
| **Other Layers** | Convolutional (`Conv2D`) | NOT SUPPORTED | No convolution layer type |
| | Dropout | NOT SUPPORTED | No dropout layer type |
| | Recurrent (`RNN`, `LSTM`) | NOT SUPPORTED | No recurrent layer type |

---

### 8.13 Roadmap: Planned Neural Network Features

Planned enhancements will transform Exatron into a complete train-and-deploy framework:

#### Dependency Order
1. **Tensor Reductions & Slicing**: Indexing (`t[r, c]`), slicing, and operations (`sum`, `mean`, `max`, `argmax`, `transpose`).
2. **Dataset Loading**: Native CSV and tensor data ingestion (`read_csv`, `load_tensor`).
3. **Automatic Differentiation**: Graph synthesis and reverse-mode automatic differentiation (autograd).
4. **Loss Functions & Optimizers**: Native primitives (`mse_loss`, `cross_entropy`, `SGD`, `Adam`).
5. **Weight Persistence**: Serializing trained parameters to disk checkpoints.
6. **Dynamic Batches & Architecture Expansion**: Flexible batch dimension `tensor<?, N>`, `Conv2D`, and `Dropout`.

> [!NOTE]
> **Design Sketch (NOT IMPLEMENTED):**
> The following speculative syntax illustrates potential future training workflows:
> ```text
> // SPECULATIVE DESIGN SKETCH — NOT IMPLEMENTED IN CURRENT COMPILER
> model Classifier {
>     input: tensor<?, 4>;
>     layer fc1: Dense(4, 8, relu);
>     layer fc2: Dense(8, 3, softmax);
> }
> fn train_model() -> void {
>     let net: Classifier = Classifier();
>     let opt: Adam = Adam(net.parameters(), 0.001);
>     let loss_fn: Loss = CrossEntropyLoss();
>     // ...
> }
> ```

---

### 8.14 Troubleshooting FAQ & Glossary

#### Frequently Asked Questions

**Q: Why does `forward()` reject a batch tensor with 10 rows?**  
**A:** If your model declares `input: tensor<1, 4>;`, the type checker expects an exact shape match of $1 \times 4$. To process a batch of 10 samples simultaneously, declare `input: tensor<10, 4>;`, or iterate over individual samples using a `for` loop.

**Q: Can I inspect or modify the weights of a layer?**  
**A:** Not directly in Exatron source code. Models only expose the `.forward()` member function. To inspect weights, compile with `--emit-cpp` and examine the C++ initialization code in the emitted `.cpp` file.

**Q: Why are my model predictions identical every time I run the program?**  
**A:** Exatron uses a deterministic Linear Congruential Generator seeded at `1337`. This ensures reproducibility across compilers and platforms.

**Q: Why are model predictions inaccurate?**  
**A:** Weights are initialized with pseudo-random numbers in $[-0.5, 0.5)$ and have not undergone gradient descent training.

#### Glossary of Terms

- **Dense Layer**: A fully connected neural network layer where every input unit connects to every output unit via a weight parameter.
- **Weights ($W$)**: Multiplicative parameters scaled against input features.
- **Bias ($b$)**: Additive parameter vector shifting activations.
- **Linear Step**: The unactivated affine transformation $x @ W + b$.
- **Activation Function**: Non-linear mathematical mapping applied to the linear step.
- **Forward Pass**: The evaluation pass calculating model outputs from input features.
- **Softmax**: Normalized exponential function converting raw logits into a categorical probability distribution.
- **Transpilation**: Source-to-source compilation translating Exatron code into C++23 source code.

---

## 9. Built-in Functions & Printing Rules

### `print(value)` Output Formatting

| Value Type | Output Format | Example Output |
|---|---|---|
| `int` | Decimal integer string | `42` |
| `float` | Standard floating-point scalar | `3.14` |
| `bool` | `true` or `false` | `true` |
| `string` | Unquoted string characters | `Hello from Exatron!` |
| `tensor<R, C>` | Formatted 2D matrix with elements at 4 decimal places | `[[1.0000, 2.0000], [3.0000, 4.0000]]` |

*Note*: Tensor output formatting is constructed inside an isolated string stream so that `std::fixed` and precision flags do not alter subsequent scalar `print` calls.

---

## 10. Installation & Toolchain

### 10.1 Supported Platforms & Distributions

Exatron is engineered and supported exclusively on Linux (x86_64). The automated installer includes native package-manager support for the three major Linux distribution families:

| Distribution Family | Tested Distributions | Package Manager | Dependencies Installed |
|---|---|---|---|
| **Ubuntu / Debian** | Ubuntu 22.04 / 24.04+, Debian 12 / 13+, Linux Mint, Pop!_OS | `apt` | `build-essential cmake git` (plus `g++-14`/`g++-13` if default lacks C++23) |
| **Fedora / RHEL** | Fedora 39 / 40+, RHEL 9+, CentOS Stream, Rocky Linux, AlmaLinux | `dnf` | `gcc-c++ make cmake git` |
| **Arch Linux** | Arch Linux, Manjaro, EndeavourOS | `pacman` | `base-devel cmake git` |
| **Windows / macOS** | Windows (native / WSL), macOS (Intel / Apple Silicon) | N/A | *Not supported yet* (Linux only for now) |

Platform detection verifies the kernel via `uname -s`. Attempting to run `install.sh` on macOS or Windows terminates execution immediately with exit code 1:
```text
Linux only for now
```

---

### 10.2 Automated Installation (`install.sh`)

The standard deployment method is the self-contained installer script located in the project root:

```bash
git clone <repo url>
cd exatron
./install.sh
```

#### Installation Modes

- **Per-User Default**: Installs binary to `~/.local/bin/exatron`. Builds as regular user without root privileges.
- **System-Wide (`--system`)**: Installs binary to `/usr/local/bin/exatron`. `sudo` is required only for package installation and the final `cmake --install` step; the build step is never executed as root.
- **Custom Prefix (`--prefix DIR`)**: Installs binary to `DIR/bin/exatron`.
- **Remote Git Source (`--repo URL [--branch NAME]`)**: Clones and builds from a specified repository without requiring a pre-existing local copy.

---

### 10.3 Execution Pipeline (Step-by-Step)

The installer operates through 6 sequential phases:

1. **[1/6] System & Environment Detection**: Checks that the host kernel is Linux (`uname -s`), parses distribution identifiers from `/etc/os-release` (`ID` and `ID_LIKE`), resolves canonical installation paths, and warns if running as root without `--system`.
2. **[2/6] Dependency & C++23 Compiler Feature Detection**: Feature-tests candidate C++ compilers (`g++`, `g++-16`, `g++-15`, `g++-14`, `g++-13`) by compiling a test C++23 generic lambda snippet with `-std=c++23`. Validates `cmake >= 3.20`, `make`, and `git`. If missing, presents required distribution packages and prompts for confirmation unless `-y` is supplied.
3. **[3/6] CMake Build Configuration**: Prepares source trees (or clones from `--repo`), allocates an isolated temporary build directory (`/tmp/exatron_build_XXXXXX`), and configures CMake:
   ```bash
   cmake -S <src> -B <builddir> -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<prefix> -DEXATRON_CXX=<detected>
   ```
4. **[4/6] Parallel Compilation**: Compiles the native `exatron` compiler binary using all available processor cores:
   ```bash
   cmake --build <builddir> -j"$(nproc)"
   ```
5. **[5/6] Binary Installation**: Installs the binary to `<prefix>/bin/exatron` via `cmake --install <builddir>` (running under `sudo` only when target directories are root-owned).
6. **[6/6] PATH Setup & Verification**: Updates user shell profiles, executes `<prefix>/bin/exatron --version`, compiles and runs an embedded hello-world smoke test program, and optionally runs the full test suite when `--run-tests` is passed.

---

### 10.4 Installer CLI Options

| Option | Purpose | Example |
|---|---|---|
| `--system` | Install system-wide to `/usr/local` (requires sudo for install step) | `./install.sh --system` |
| `--prefix DIR` | Install to custom prefix directory (default: `~/.local`) | `./install.sh --prefix /opt/exatron` |
| `-y`, `--yes` | Non-interactive mode; automatically confirm dependency package installations | `./install.sh -y` |
| `--no-deps` | Skip package manager checks and package installations | `./install.sh --no-deps` |
| `--run-tests` | Run the full test suite (`bash tests/run_tests.sh`) after build & install | `./install.sh --run-tests` |
| `--uninstall` | Cleanly uninstall binary, shell PATH blocks, and fish configuration | `./install.sh --uninstall` |
| `--dry-run` | Print every command that would be executed without running it | `./install.sh --dry-run` |
| `--repo URL` | Clone and build from a remote Git repository URL | `./install.sh --repo https://github.com/exatron-lang/exatron.git` |
| `--branch NAME` | Branch or tag to clone when using `--repo` | `./install.sh --repo ... --branch main` |
| `-h`, `--help` | Display command-line usage information and exit | `./install.sh --help` |

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

### 10.5 Shell PATH Configuration & Idempotency Rules

When installing in user mode (`~/.local` or `--prefix`), the installer configures shell initialization files so `exatron` can be invoked from any working directory:

1. **Guarded Block in POSIX Shells**: Appends a single guarded block to `~/.bashrc`, `~/.profile`, and `~/.zshrc` (creating them if absent):
   ```bash
   # >>> exatron >>>
   export PATH="<prefix>/bin:$PATH"
   # <<< exatron <<<
   ```
2. **Fish Shell**: Writes `~/.config/fish/conf.d/exatron.fish`:
   ```fish
   fish_add_path "<prefix>/bin"
   ```
3. **Idempotency Guarantee**: The installer strips any existing `# >>> exatron >>>` block using `sed` before appending. Re-running `install.sh` updates the block in place and never creates duplicates.
4. **Shell Session Environment**: Shell scripts cannot alter the environment of the parent shell process. To use `exatron` immediately in the current session, run:
   ```bash
   source ~/.bashrc
   # or: export PATH="$HOME/.local/bin:$PATH"
   ```
   Or open a new terminal window.

---

### 10.6 Verification, Upgrades & Uninstallation

#### Verification
Verify the installation by running the version check and compiling a smoke-test program:
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

#### Upgrades
To upgrade Exatron to the latest revision, pull git updates and re-run the installer:
```bash
git pull
./install.sh
```

#### Clean Uninstallation
To completely remove Exatron:
```bash
./install.sh --uninstall
```
Or for custom installations:
```bash
./install.sh --uninstall --prefix /opt/exatron
```
Uninstallation removes `<prefix>/bin/exatron`, strips marked PATH blocks from `~/.bashrc`, `~/.profile`, and `~/.zshrc`, and deletes `exatron.fish`. System packages installed as dependencies remain untouched.

---

### 10.7 Manual Build & Installation Procedure

For unsupported Linux distributions or custom build environments:

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

### 10.8 Persistent Runtime Backend Requirement

Exatron is a transpiled programming language. The compiler generates standalone, self-contained C++23 source files that embed the `exatron_rt` tensor library and then invokes `g++` (or the compiler specified by `EXATRON_CXX`) to produce a native ELF executable binary. Consequently, GCC `g++` is both a build dependency and a permanent runtime execution requirement.

---

### 10.9 Troubleshooting Guide

- **`g++ too old for C++23`**: Exatron requires full C++23 support (`-std=c++23`). On older distributions such as Ubuntu 22.04 LTS (where default `g++` is version 11), install `g++-14` or `g++-13`:
  ```bash
  sudo apt update && sudo apt install -y g++-14
  EXATRON_CXX=g++-14 ./install.sh
  ```
- **`CMake >= 3.20 is required`**: If your distribution provides an older CMake, upgrade via your package manager's backports, via Snap (`snap install cmake --classic`), or download the official precompiled binary release from [cmake.org](https://cmake.org/download/).
- **`exatron: command not found` after install**: Your current shell session has not reloaded `PATH`. Run `source ~/.bashrc` or `export PATH="$HOME/.local/bin:$PATH"`, or restart your terminal.
- **`sudo missing or permission denied`**: Installing system-wide (`--system`) writes to `/usr/local/bin`, requiring root privileges. If `sudo` is unavailable, run without `--system` to install to `~/.local/bin`, or switch to `root` (`su -`) prior to running `./install.sh --system`.
- **`Package manager lock errors`**: Wait for any background system updates (e.g., `unattended-upgrades`) to finish releasing `/var/lib/dpkg/lock-frontend` or `/var/lib/pacman/db.lck` before running `./install.sh`.
- **`Previewing with --dry-run`**: Print all commands without making filesystem changes:
  ```bash
  ./install.sh --dry-run
  ```
  Distro simulation is also supported:
  ```bash
  EXATRON_FORCE_DISTRO=ubuntu ./install.sh --dry-run
  EXATRON_FORCE_DISTRO=fedora ./install.sh --dry-run
  EXATRON_FORCE_DISTRO=arch ./install.sh --dry-run
  ```

---

## 11. Compiler CLI Reference

```text
exatron <input.ex> [-o <output_binary>] [--emit-cpp] [--dump-tokens] [--run] [-h|--help] [--version]
```

### Flags & Options
- `<input.ex>`: Path to the Exatron source file to compile.
- `-o <output_binary>`: Destination path of the compiled executable binary. Defaults to `<stem>` in the current working directory.
- `--emit-cpp`: Preserves the generated C++ source file (`<output_binary>.cpp` or `<stem>.cpp`) and prints its path to stdout. Without this flag, intermediate C++ source files are stored in a temporary directory and unlinked immediately after native compilation.
- `--dump-tokens`: Lexes the input, prints the token sequence, and terminates immediately.
- `--run`: Executes the compiled binary immediately after native linking and forwards its exit code.
- `-h`, `--help`: Prints command syntax and option summaries.
- `--version`: Displays compiler version information (`Exatron compiler v<version>`) and exits immediately with code 0.

### Environment Variables
- `EXATRON_CXX`: Specifies the C++ compiler executable used by the Exatron backend to compile generated C++ source files into native executables.
  **Precedence Order**:
  1. Environment variable `EXATRON_CXX` (if non-empty)
  2. Compile-time definition `EXATRON_DEFAULT_CXX` (set at CMake configure time via `-DEXATRON_CXX=<cxx>`)
  3. Default fallback `"g++"`

  **Example**:
  ```bash
  EXATRON_CXX=g++-14 exatron my_program.ex -o my_program --run
  ```

### Exit Codes
- `0`: Successful compilation, successful execution of `--dump-tokens`, `--help`, or `--version`, or target program exited with code 0 when run via `--run`.
- `1`: Lexer error, syntax parse error, type checking error, or backend C++ compilation failure.
- `<N>`: When `--run` is specified, the non-zero exit code of the compiled program is propagated directly.

---

## 12. Compilation Model & Runtime Architecture

### 12.1 The `ex_` Name Mangling Scheme
To prevent namespace collisions with C++ keywords (`class`, `new`, `delete`, `template`, `namespace`) or C standard library symbols (`abs`, `exit`), the code generator prefixes all user-defined identifiers with `ex_`:
- Variables: `let new: int = 5;` $\to$ `int64_t ex_new = static_cast<int64_t>(5);`
- Functions: `fn abs(x: int) -> int` $\to$ `int64_t ex_abs(int64_t ex_x)`
- Models: `model Net` $\to$ `struct ex_Net`

**Exceptions**:
- `main` remains `int main()` to serve as the C++ runtime entry point.
- The model evaluation method `forward` remains unmangled.
- All runtime components reside in the isolated `exatron_rt` namespace.

### 12.2 The `exatron_rt` Embedded Runtime
The code generator embeds the zero-dependency `exatron_rt` namespace at the head of every generated C++ file:
- `exatron_rt::Tensor`: Flat contiguous storage (`std::vector<float>`) with row-major 2D indexing (`data[r * cols + c]`).
- `exatron_rt::DenseLayer`: Owns `weights` and `bias` tensors, providing a `.linear_step(x)` method ($xW + b$).
- `exatron_rt::idiv` & `exatron_rt::imod`: Safety wrappers intercepting division by zero and invoking `std::exit(1)`.
- Activations: Vectorized activation routines (`relu`, `sigmoid`, `tanh_act`, `softmax`, `linear`).

### 12.3 Backend Compiler Selection & Invocation
The Exatron driver compiles generated C++ code into a native ELF executable by invoking a backend C++23 compiler:

1. **Compiler Selection Hierarchy**:
   - The driver queries `std::getenv("EXATRON_CXX")`. If set and non-empty, its value is selected.
   - If not set, the driver checks the macro `EXATRON_DEFAULT_CXX` configured at build time via CMake.
   - If neither is provided, the driver falls back to `"g++"`.

2. **Native Invocation Command**:
   ```bash
   "$EXATRON_CXX" -std=c++23 -O2 <intermediate.cpp> -o <output_binary> 2>&1
   ```

3. **Temporary File Management**:
   - Unless `--emit-cpp` is passed, the intermediate C++ file is written to the system temporary directory with a randomized prefix (e.g. `/tmp/exatron_<random>_<stem>.cpp`).
   - The temporary C++ source is automatically unlinked immediately after backend compilation finishes.

---

## 13. Error Reference & Diagnostics

Exatron emits diagnostics formatted with source coordinates:
`[<Category> Error] Line <Line>, Col <Col>: <Message>`

### 13.1 Lexical Errors (`[Lexer Error]`)
Triggered when the lexer encounters unrecognized characters or unterminated string literals.

**Triggering Program (`docs/examples/err_sample_lexer.ex`):**
```text
fn main() -> int {
    let msg: string = "unterminated string;
    return 0;
}
```
**Diagnostic Output:**
```text
[Lexer Error] Line 2, Col 23: Unterminated string literal
```
**Fix:** Provide a closing double quote on the string literal.

---

### 13.2 Syntax Errors (`[Parse Error]`)
Triggered when the token stream violates the language grammar or tensor dimensionality constraints.

**Triggering Program (`docs/examples/err_sample_parse.ex`):**
```text
fn main() -> int {
    let t: tensor<1, 2, 3> = [[1.0]];
    return 0;
}
```
**Diagnostic Output:**
```text
[Parse Error] Line 2, Col 25: Tensors support at most 2 dimensions
```
**Fix:** Restrict tensor declarations to rank 1 (`tensor<N>`) or rank 2 (`tensor<R, C>`).

---

### 13.3 Type Errors (`[Type Error]`)
Triggered by mismatched scalar types, dimension incompatibilities, missing return paths, or undeclared symbols.

**Triggering Program (`docs/examples/err_sample_type.ex`):**
```text
fn main() -> int {
    let x: int = 4.5;
    return 0;
}
```
**Diagnostic Output:**
```text
[Type Error] Line 2, Col 5: Type mismatch in let declaration: declared int but initializer has type float
```
**Fix:** Match the initializer expression type to the declared type (`let x: float = 4.5;` or `let x: int = 4;`).

---

### 13.4 Runtime Errors
Triggered during program execution when encountering an arithmetic division or modulo by zero.

**Triggering Program (`docs/examples/err_sample_runtime.ex`):**
```text
fn main() -> int {
    let a: int = 10;
    let b: int = 0;
    print(a / b);
    return 0;
}
```
**Diagnostic Output:**
```text
Runtime Error: division by zero
```
**Fix:** Check that the denominator is not zero prior to division.

---

## 14. Formal EBNF Grammar

```ebnf
Program        ::= (ModelDecl | FnDecl)*

ModelDecl      ::= "model" IDENT "{" InputDecl LayerDecl+ "}"
InputDecl      ::= "input" ":" TensorType ";"
LayerDecl      ::= "layer" IDENT ":" LayerSpec ";"
LayerSpec      ::= "Dense" "(" INT_LIT "," INT_LIT "," Activation ")"
Activation     ::= "relu" | "sigmoid" | "tanh" | "softmax" | "linear"

FnDecl         ::= "fn" IDENT "(" ParamList? ")" "->" Type Block
ParamList      ::= Param ("," Param)*
Param          ::= IDENT ":" Type

Type           ::= "int" | "float" | "bool" | "string" | "void" | TensorType | IDENT
TensorType     ::= "tensor" "<" INT_LIT ("," INT_LIT)? ">"

Block          ::= "{" Stmt* "}"
Stmt           ::= LetStmt | AssignStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt | PrintStmt | ExprStmt
LetStmt        ::= "let" IDENT ":" Type "=" Expr ";"
AssignStmt     ::= IDENT "=" Expr ";"
IfStmt         ::= "if" "(" Expr ")" Block ( "else" ( IfStmt | Block ) )?
WhileStmt      ::= "while" "(" Expr ")" Block
ForStmt        ::= "for" "(" IDENT "in" Expr ".." Expr ")" Block
ReturnStmt     ::= "return" Expr? ";"
PrintStmt      ::= "print" "(" Expr ")" ";"
ExprStmt       ::= Expr ";"

Expr           ::= LogicOr
LogicOr        ::= LogicAnd ("||" LogicAnd)*
LogicAnd       ::= Equality ("&&" Equality)*
Equality       ::= Comparison (("==" | "!=") Comparison)*
Comparison     ::= Term (("<" | "<=" | ">" | ">=") Term)*
Term           ::= Factor (("+" | "-") Factor)*
Factor         ::= Unary (("*" | "/" | "%" | "@") Unary)*
Unary          ::= ("-" | "!") Unary | Postfix
Postfix        ::= Primary ("." IDENT "(" ArgList? ")" | "(" ArgList? ")")*
ArgList        ::= Expr ("," Expr)*
Primary        ::= INT_LIT | FLOAT_LIT | STRING_LIT | "true" | "false" | IDENT | TensorInit | TensorLit | "(" Expr ")"
TensorInit     ::= ("zeros" | "ones" | "rand") "(" INT_LIT "," INT_LIT ")"
TensorLit      ::= "[" Expr ("," Expr)* "]"
```

---

## 15. Known Limitations

1. **Non-Overridable Keywords**: Words such as `input`, `layer`, `linear`, `relu`, `sigmoid`, `tanh`, `softmax`, `zeros`, `ones`, `rand`, `Dense`, `model`, and `print` are reserved across all contexts.
2. **No Implicit Numerical Promotion**: Arithmetic between `int` and `float` without explicit matching types is forbidden.
3. **Rank Invariance**: Tensors are limited to rank 1 or rank 2. Higher-order tensors (e.g. 3D batches) are not supported.
4. **Forward Evaluation Only**: Models do not compute gradients or support backward propagation.
5. **Fixed Static Inputs**: Model input dimensions are fixed at declaration time; dynamic batch dimensions are unsupported.
6. **Single Diagnostic Reporting**: The compiler halts execution immediately upon detecting the first lexical, parse, or type error.

---

## 16. Complete Example Programs

### Example 1: General-Purpose Computations (`docs/examples/full_general.ex`)
Demonstrates functions, recursion, mutable while loops, and for loops:

```text
fn gcd(a: int, b: int) -> int {
    while (b != 0) {
        let temp: int = b;
        b = a % b;
        a = temp;
    }
    return a;
}

fn fibonacci(n: int) -> int {
    if (n <= 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    }
    let prev: int = 0;
    let curr: int = 1;
    for (i in 2..n + 1) {
        let nxt: int = prev + curr;
        prev = curr;
        curr = nxt;
    }
    return curr;
}

fn main() -> int {
    print("GCD of 48 and 18:");
    print(gcd(48, 18));

    print("Fibonacci(10):");
    print(fibonacci(10));
    return 0;
}
```

**Real Output:**
```text
GCD of 48 and 18:
6
Fibonacci(10):
55
```

---

### Example 2: Tensor Algebra & Matrix Multiplication (`docs/examples/full_tensor.ex`)
Demonstrates 2D matrix literals, element-wise arithmetic, `@` multiplication, and random initialization:

```text
fn main() -> int {
    let A: tensor<2, 2> = [[2.0, 1.0], [0.0, 3.0]];
    let B: tensor<2, 2> = [[1.0, 2.0], [3.0, 4.0]];

    print("A + B:");
    print(A + B);

    print("A - B:");
    print(A - B);

    print("A * B (element-wise):");
    print(A * B);

    print("A @ B (matrix multiplication):");
    print(A @ B);

    let R: tensor<2, 3> = rand(2, 3);
    print("Deterministic random tensor (2x3):");
    print(R);

    return 0;
}
```

**Real Output:**
```text
A + B:
[[3.0000, 3.0000], [3.0000, 7.0000]]
A - B:
[[1.0000, -1.0000], [-3.0000, -1.0000]]
A * B (element-wise):
[[2.0000, 2.0000], [0.0000, 12.0000]]
A @ B (matrix multiplication):
[[5.0000, 8.0000], [9.0000, 12.0000]]
Deterministic random tensor (2x3):
[[0.2542, 0.0495, -0.2255], [-0.3413, -0.0505, -0.1888]]
```

---

### Example 3: Multi-Layer Neural Network Forward Pass (`docs/examples/full_neural_network.ex`)
Demonstrates declaring a multi-layer neural network with `relu`, `tanh`, and `softmax` activations, followed by instantiation and forward evaluation:

```text
model PerceptronClassifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 4, tanh);
    layer fc3: Dense(4, 3, softmax);
}

fn main() -> int {
    let net: PerceptronClassifier = PerceptronClassifier();
    let features: tensor<1, 4> = [[0.8, -0.5, 1.2, 0.1]];
    let probabilities: tensor<1, 3> = net.forward(features);

    print("Input feature vector:");
    print(features);

    print("Predicted class probabilities:");
    print(probabilities);

    return 0;
}
```

**Real Output:**
```text
Input feature vector:
[[0.8000, -0.5000, 1.2000, 0.1000]]
Predicted class probabilities:
[[0.3298, 0.3346, 0.3356]]
```
*(Notice: $0.3298 + 0.3346 + 0.3356 = 1.0000$)*

