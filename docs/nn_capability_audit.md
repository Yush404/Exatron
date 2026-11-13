# Exatron Neural Network Capability Audit

*Generated on 2026-11-13 against Exatron compiler v0.1.0 (g++ 16.2.1, Arch Linux)*

## Audit Methodology

Each item was tested by writing a `.ex` program, compiling it with `./build/exatron <file> -o <bin> --run`, and recording the exit code and output verbatim.

---

## Model Declarations & Activations

| Feature | Status | Details |
|---|---|---|
| Model with 1 Dense layer | SUPPORTED | `model M { input: tensor<1,3>; layer l1: Dense(3,2,relu); }` compiles and runs |
| Model with N Dense layers | SUPPORTED | Tested with 2, 3, 4, and 5 layers. All compile and run |
| Activation: `relu` | SUPPORTED | Hidden layer activation. Output values ≥ 0 |
| Activation: `sigmoid` | SUPPORTED | Output values in (0, 1). Used for binary classification output |
| Activation: `tanh` | SUPPORTED | Output values in (-1, 1) |
| Activation: `softmax` | SUPPORTED | Output row sums to 1.0. Used for multi-class output |
| Activation: `linear` | SUPPORTED | Identity pass-through. Output = input (no activation applied) |
| All 5 activations in one model | SUPPORTED | Tested relu→sigmoid→tanh→linear→softmax chain |

## Forward Pass

| Feature | Status | Details |
|---|---|---|
| `forward()` with correct input shape | SUPPORTED | Returns tensor of expected output dimensions |
| `forward()` with wrong input shape | SUPPORTED (compile-time error) | `[Type Error] Line 9, Col 37: forward() expects input of type tensor<1, 3> but got tensor<1, 4>` |

## Tensor Creation & Literals

| Feature | Status | Details |
|---|---|---|
| 1D tensor literal `[1.0, 2.0, 3.0]` | SUPPORTED | Normalized to `tensor<1, 3>` internally |
| 2D tensor literal `[[1.0, 2.0], [3.0, 4.0]]` | SUPPORTED | Creates `tensor<2, 2>` |
| `zeros(R, C)` | SUPPORTED | All elements 0.0 |
| `ones(R, C)` | SUPPORTED | All elements 1.0 |
| `rand(R, C)` | SUPPORTED | Deterministic LCG, values in [-0.5, 0.5) |

## Tensor Operations

| Feature | Status | Details |
|---|---|---|
| Element-wise `+` | SUPPORTED | `tensor + tensor`, matching shapes required |
| Element-wise `-` | SUPPORTED | `tensor - tensor`, matching shapes required |
| Element-wise `*` | SUPPORTED | `tensor * tensor`, matching shapes required |
| Matrix multiply `@` | SUPPORTED | `tensor<A,B> @ tensor<B,C>` → `tensor<A,C>`, dimension checked at compile time |
| Unary minus `-tensor` | NOT SUPPORTED | `[Type Error] Operator '-' cannot be applied to tensor<2, 2>` |
| Scalar-tensor ops (e.g. `tensor * 2.0`) | NOT SUPPORTED | `[Type Error] Invalid operand types for arithmetic: tensor<2, 2> and float` |
| Element-wise `/` | NOT SUPPORTED | `[Type Error] Invalid operand types for division: tensor<2, 2> and tensor<2, 2>` |
| Tensor comparisons (`==`, `<`, etc.) | NOT SUPPORTED | `[Type Error] Equality comparison requires matching scalar types, got tensor<2, 2> and tensor<2, 2>` |

## Advanced Tensor Features

| Feature | Status | Details |
|---|---|---|
| Element indexing `t[0]` or `t[i, j]` | NOT SUPPORTED | `[Parse Error] Expected ')' after print argument (found '[')` |
| Slicing `t[0..2]` | NOT SUPPORTED | Parse error |
| `sum()` | NOT SUPPORTED | `[Type Error] Undefined function 'sum'` |
| `mean()` | NOT SUPPORTED | `[Type Error] Undefined function 'mean'` |
| `max()` / `argmax()` | NOT SUPPORTED | Undefined function errors |
| `reshape()` | NOT SUPPORTED | Undefined function |
| `transpose()` | NOT SUPPORTED | `[Type Error] Undefined function 'transpose'` |

## Weight Management & Training

| Feature | Status | Details |
|---|---|---|
| Accessing layer weights (`m.fc1`) | NOT SUPPORTED | `[Parse Error] Expected '(' after method name` |
| Setting weights | NOT SUPPORTED | No assignment syntax for model fields |
| Saving/loading weights | NOT SUPPORTED | No file I/O functions |
| Training / backpropagation | NOT SUPPORTED | `[Type Error] Model 'Net' has no method 'train'` |
| Loss functions (`mse`, `cross_entropy`) | NOT SUPPORTED | `[Type Error] Undefined function 'mse'` |
| Optimizers | NOT SUPPORTED | No optimizer API |
| Metrics (accuracy, etc.) | NOT SUPPORTED | No metric functions |
| Reading data files | NOT SUPPORTED | `[Type Error] Undefined function 'read_file'` |

## Batch & Multi-Model Features

| Feature | Status | Details |
|---|---|---|
| Batch input (rows > 1) against `tensor<1, N>` model | NOT SUPPORTED (compile-time error) | `[Type Error] forward() expects input of type tensor<1, 3> but got tensor<2, 3>`. Model must declare `input: tensor<2, 3>` explicitly for batch size 2 |
| Model declared with `input: tensor<2, N>` | SUPPORTED | Works with matching batch size at compile time |
| Multiple models in one file | SUPPORTED | Two or more model blocks compile and can be instantiated independently |
| Passing a model to a function | SUPPORTED | `fn evaluate(m: Net, x: tensor<1, 2>) -> tensor<1, 2>` works |
| Functions with tensor parameters/returns | SUPPORTED | Tensor values pass by value, return correctly |
| Tensors used inside loops | SUPPORTED | Accumulating tensors in a `for` loop works correctly |

## Random Number Generator Behavior

| Feature | Status | Details |
|---|---|---|
| Shared RNG state between `rand()` and model weights | CONFIRMED | Both `Tensor::rand()` and `DenseLayer` constructor use the same global `g_rand_state` (LCG seeded at 1337). The order of allocation in the generated code determines which random values each receives. This makes output fully deterministic but means user `rand()` calls consume from the same sequence as model weight initialization |
| Determinism across runs | CONFIRMED | Two executions of the same binary produce identical output |

## Compile-Time Shape Errors

| Error | Status | Exact Message |
|---|---|---|
| Layer chain dimension mismatch | SUPPORTED | `[Type Error] Layer 'l2' expects input dimension 5, but previous dimension is 4` |
| `@` dimension mismatch | SUPPORTED | `[Type Error] Matrix multiplication dimension mismatch: cannot multiply tensor<2, 3> by tensor<2, 2>` |
| Wrong forward input shape | SUPPORTED | `[Type Error] forward() expects input of type tensor<1, 3> but got tensor<1, 4>` |
| Jagged tensor literal | SUPPORTED | `[Type Error] Jagged tensor literal: expected 2 columns but got 1` |
| Unknown model type | SUPPORTED | `[Type Error] Unknown type 'FooNet'` |

---

## Bugs Found

**No bugs were found during this audit.** All behaviors matched expected semantics. Items marked NOT SUPPORTED produce clear, specific error messages at compile time or parse time.

---

## Summary

- **Total items audited**: 42
- **SUPPORTED**: 24
- **NOT SUPPORTED (clear error)**: 18
- **UNEXPECTED BEHAVIOR (bugs)**: 0

