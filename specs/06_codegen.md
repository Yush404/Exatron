# Feature 06: C++23 Transpiler Code Generator & Embedded Tensor/NN Runtime

## 1. Objective
Implement the C++23 code generator in `include/runtime_emitter.hpp`, `include/codegen.hpp`, and `src/codegen.cpp`. The code generator translates a type-checked `exatron::Program` AST into a single, self-contained, zero-dependency C++23 source file (`.cpp`) that compiles cleanly with `g++ -std=c++23 -O2`.

## 2. Embedded Tensor & Neural Network Runtime (`include/runtime_emitter.hpp`)
Create `include/runtime_emitter.hpp` containing a raw string literal `EXATRON_RUNTIME_HEADER` that is emitted at the top of every generated `.cpp` file. It must provide:

1. **Standard Includes**:
   `<iostream>`, `<vector>`, `<string>`, `<cmath>`, `<iomanip>`, `<cstdint>`, `<stdexcept>`, `<algorithm>`
2. **`exatron_rt::Tensor` Struct**:
   - Fields: `int rows`, `int cols`, `std::vector<float> data`.
   - Constructors:
     - `Tensor(int r, int c, float fill = 0.0f)`
     - `Tensor(int r, int c, std::initializer_list<float> vals)`
   - Element access: `float& at(int r, int c) { return data[r * cols + c]; }` and `float at(int r, int c) const`.
   - Static initializers:
     - `static Tensor zeros(int r, int c)`
     - `static Tensor ones(int r, int c)`
     - `static Tensor rand(int r, int c)`: uses a deterministic LCG (`uint32_t seed = 1337`) so test outputs are reproducible across runs, mapping values to `[-0.5f, 0.5f]`.
   - Element-wise operators: `+`, `-`, `*` between two `Tensor` objects of identical shape, plus row-broadcast bias addition `add_bias(const Tensor& x, const Tensor& bias_1xN)`.
   - Matrix Multiplication:
     ```cpp
     inline Tensor matmul(const Tensor& a, const Tensor& b) {
         Tensor out(a.rows, b.cols, 0.0f);
         for (int i = 0; i < a.rows; ++i) {
             for (int k = 0; k < a.cols; ++k) {
                 float aik = a.at(i, k);
                 for (int j = 0; j < b.cols; ++j) {
                     out.at(i, j) += aik * b.at(k, j);
                 }
             }
         }
         return out;
     }
     ```
   - Activation Functions (`relu`, `sigmoid`, `tanh_act`, `softmax`, `linear`):
     - `relu(const Tensor& x)`: `std::max(0.0f, v)` element-wise.
     - `sigmoid(const Tensor& x)`: `1.0f / (1.0f + std::exp(-v))` element-wise.
     - `tanh_act(const Tensor& x)`: `std::tanh(v)` element-wise.
     - `softmax(const Tensor& x)`: Row-wise numerically stable softmax (subtract row max, exponentiate, divide by row sum).
     - `linear(const Tensor& x)`: Identity return `x`.
   - `DenseLayer` Struct:
     ```cpp
     struct DenseLayer {
         Tensor weights; // shape: in_features x out_features
         Tensor bias;    // shape: 1 x out_features
         DenseLayer(int in_f, int out_f)
             : weights(Tensor::rand(in_f, out_f)), bias(Tensor::zeros(1, out_f)) {}
         Tensor linear_step(const Tensor& input) const {
             return add_bias(matmul(input, weights), bias);
         }
     };
     ```
   - Printing helper `exatron_rt::print_val(...)`:
     - Overloads for `int64_t`, `float`, `bool` (prints `true`/`false`), `const std::string&`, and `const Tensor&` (prints formatted 2D bracketed matrix e.g. `[[0.1234, 0.5678]]` with 4 decimal places).

## 3. Code Generator Interface (`include/codegen.hpp` & `src/codegen.cpp`)

```cpp
#pragma once
#include "ast.hpp"
#include <sstream>
#include <string>

namespace exatron {

class CodeGenerator {
public:
    std::string generate(const Program& program);

private:
    std::ostringstream out_;
    int indent_level_ = 0;

    void emit_indent();
    void emit_model(const ModelDecl& model);
    void emit_fn_decl(const FnDecl& fn);
    void emit_block(const BlockStmt& block, bool include_braces = true);
    void emit_stmt(const Stmt& stmt);
    void emit_expr(const Expr& expr);
    std::string map_type(const TypeInfo& type) const;
};

} // namespace exatron
```

## 4. Translation Rules (`src/codegen.cpp`)

1. **Type Mapping (`map_type`)**:
   - `BaseType::Int` -> `"int64_t"`
   - `BaseType::Float` -> `"float"`
   - `BaseType::Bool` -> `"bool"`
   - `BaseType::String` -> `"std::string"`
   - `BaseType::Void` -> `"void"`
   - `BaseType::Tensor` -> `"exatron_rt::Tensor"`
   - `BaseType::Model` -> `type.model_name`

2. **Model Translation (`emit_model`)**:
   For each `ModelDecl` (e.g., `Classifier`), emit a C++ `struct Classifier`:
   ```cpp
   struct Classifier {
       exatron_rt::DenseLayer fc1{4, 8};
       exatron_rt::DenseLayer fc2{8, 2};

       exatron_rt::Tensor forward(const exatron_rt::Tensor& input) const {
           exatron_rt::Tensor x = input;
           x = exatron_rt::relu(fc1.linear_step(x));
           x = exatron_rt::softmax(fc2.linear_step(x));
           return x;
       }
   };
   ```

3. **Statement Translation (`emit_stmt`)**:
   - `LetStmt`: `<map_type> <name> = <emit_expr>;\n`
   - `AssignStmt`: `<name> = <emit_expr>;\n`
   - `IfStmt`: `if (<cond>) { ... } else { ... }\n`
   - `WhileStmt`: `while (<cond>) { ... }\n`
   - `ForStmt`: `for (int64_t <var> = <start>; <var> < <end>; ++<var>) { ... }\n`
   - `ReturnStmt`: `return <expr>;\n`
   - `PrintStmt`: `exatron_rt::print_val(<expr>);\n`
   - `ExprStmt`: `<expr>;\n`

4. **Expression Translation (`emit_expr`)**:
   - `BinaryExpr` with `TokenType::MatMul` (`@`): emit `exatron_rt::matmul(<lhs>, <rhs>)`.
   - Other `BinaryExpr`: emit `(<lhs> <op> <rhs>)`.
   - `TensorInitExpr`: emit `exatron_rt::Tensor::<kind>(<rows>, <cols>)`.
   - `TensorLitExpr`: flatten the 1D or 2D literal elements into `exatron_rt::Tensor(<rows>, <cols>, { <comma-separated float expressions> })`.
   - `MemberCallExpr`: emit `<object>.<method>(<args>)`.
   - `CallExpr`: emit `<callee>(<args>)`.
   - Note on `fn main() -> int`: In C++, `main()` must return `int`, so special-case `fn.name == "main"` to emit `int main()` while keeping `int64_t` for all other `int` functions.

## 5. Verification Step
Compile with `cmake --build build` and verify that generating C++ code for a program with both general-purpose functions and a `model` declaration produces valid C++23 code that `g++ -std=c++23` compiles into a working binary.
