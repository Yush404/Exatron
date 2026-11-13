# Feature 05: Semantic Analysis, Symbol Table & Tensor Dimension Type Checker

## 1. Objective
Implement static semantic analysis and type checking in `include/type_checker.hpp` and `src/type_checker.cpp`. This pass validates variable scopes, function signatures, control-flow condition types, tensor literal shapes, matrix multiplication (`@`) inner-dimension compatibility, and neural network `model` layer-to-layer dimension chaining.

## 2. Type Checker Interface (`include/type_checker.hpp`)
Create `include/type_checker.hpp`:

```cpp
#pragma once
#include "ast.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace exatron {

class TypeError : public std::runtime_error {
public:
    SourceLoc loc;
    TypeError(const std::string& msg, SourceLoc l)
        : std::runtime_error(msg), loc(l) {}
};

struct FnSignature {
    std::vector<TypeInfo> param_types;
    TypeInfo return_type;
};

struct ModelSignature {
    std::string name;
    TypeInfo input_type;
    TypeInfo output_type;
};

class TypeChecker {
public:
    void check_program(Program& program);

private:
    std::unordered_map<std::string, FnSignature> functions_;
    std::unordered_map<std::string, ModelSignature> models_;
    std::vector<std::unordered_map<std::string, TypeInfo>> scopes_;
    TypeInfo current_return_type_{BaseType::Void};

    void push_scope();
    void pop_scope();
    void declare_var(const std::string& name, const TypeInfo& type, SourceLoc loc);
    TypeInfo lookup_var(const std::string& name, SourceLoc loc) const;

    void check_model(ModelDecl& model);
    void check_fn(FnDecl& fn);
    void check_block(BlockStmt& block);
    void check_stmt(Stmt& stmt);
    TypeInfo check_expr(Expr& expr);
};

} // namespace exatron
```

## 3. Validation Rules (`src/type_checker.cpp`)

### 3.1 Pass 1: Register Models and Function Signatures
1. **Validate Models (`check_model`)**:
   - Ensure `model.input_type.base == BaseType::Tensor` and convert 1D `{N}` shapes to 2D `{1, N}` so every tensor in the pipeline has rank 2 `{rows, cols}`.
   - Track `int current_features = model.input_type.shape[1]`.
   - Iterate through `model.layers`:
     - Verify `layer.in_features == current_features`. If not, throw `TypeError`:
       `"[Type Error] Line <line>, Col <col>: Layer '<layer.name>' expects input dimension <layer.in_features>, but previous dimension is <current_features>"`
     - Update `current_features = layer.out_features`.
   - Set `model.output_type = TypeInfo{BaseType::Tensor, {model.input_type.shape[0], current_features}, ""}`.
   - Register `ModelSignature{model.name, model.input_type, model.output_type}` in `models_`.
2. **Register Functions**:
   - Register every `FnDecl` in `functions_` (checking for duplicate function names).
   - Verify a `main` function exists with signature `fn main() -> int`.

### 3.2 Pass 2: Check Function Bodies & Statements
For each `FnDecl`:
- Set `current_return_type_ = fn.return_type`.
- Push a new lexical scope and insert all function parameters.
- Check all statements in `fn.body`:
  - `LetStmt`: Normalize 1D `tensor<N>` to `{1, N}`. Evaluate `init_type = check_expr(*stmt.init)`. Verify `init_type == stmt.declared_type`. Declare variable in current scope.
  - `AssignStmt`: Lookup variable type via `lookup_var`. Evaluate RHS type and verify equality.
  - `IfStmt` / `WhileStmt`: Verify condition expression has `BaseType::Bool`. Check branches in nested scopes.
  - `ForStmt`: Verify `start_expr` and `end_expr` both have `BaseType::Int`. Push scope, declare `loop_var` as `BaseType::Int`, and check body.
  - `ReturnStmt`: Verify returned expression matches `current_return_type_`.
  - `PrintStmt`: Allow any non-`Void`, non-`Model` expression (`Int`, `Float`, `Bool`, `String`, `Tensor`).

### 3.3 Expression Type Inference & Tensor Shape Checking (`check_expr`)
Every `check_expr` call must set `expr.inferred_type` before returning:
1. **Literals**:
   - `IntLitExpr` -> `Int`
   - `FloatLitExpr` -> `Float`
   - `BoolLitExpr` -> `Bool`
   - `StringLitExpr` -> `String`
2. **Tensor Initializers (`TensorInitExpr`)**:
   - Verify `rows > 0 && cols > 0`.
   - Set `inferred_type = TypeInfo{BaseType::Tensor, {rows, cols}, ""}`.
3. **Tensor Literals (`TensorLitExpr`)**:
   - Support 1D row vector `[1.0, 2.0, 3.0]` (infer type `tensor<1, N>` where all elements are `float` or implicitly promotable `int`) and 2D matrix `[[1.0, 2.0], [3.0, 4.0]]` (infer type `tensor<R, C>` where every row has the same column length `C`).
   - Throw `TypeError` if rows have jagged/mismatched lengths.
4. **Binary Operators (`BinaryExpr`)**:
   - Arithmetic `+`, `-`, `*`, `/`:
     - Scalar `int op int -> int`, `float op float -> float`.
     - Element-wise Tensor `tensor<R, C> op tensor<R, C> -> tensor<R, C>` for `+`, `-`, `*`.
   - Modulo `%`: `int % int -> int`.
   - **Matrix Multiplication `@` (`TokenType::MatMul`)**:
     - Both LHS and RHS must be `BaseType::Tensor` with 2D shapes `{M, K}` and `{K2, N}`.
     - Verify inner dimensions match: `K == K2`. If `K != K2`, throw `TypeError`:
       `"[Type Error] Line <line>, Col <col>: Matrix multiplication dimension mismatch: cannot multiply tensor<M, K> by tensor<K2, N>"`
     - Resulting `inferred_type` is `tensor<M, N>`.
   - Comparisons `<, <=, >, >=`: `int` or `float` operands of matching type -> `bool`.
   - Equality `==, !=`: matching scalar types -> `bool`.
   - Logical `&&, ||`: `bool` and `bool` -> `bool`.
5. **Function Calls & Model Constructors (`CallExpr`)**:
   - If `callee` is in `models_`: verify `args.size() == 0` and return `TypeInfo{BaseType::Model, {}, callee}`.
   - Else if `callee` is in `functions_`: verify argument count and parameter types match `FnSignature`, and return `sig.return_type`.
6. **Model Forward Pass (`MemberCallExpr`)**:
   - Verify `object` has `BaseType::Model`. Lookup `ModelSignature` in `models_`.
   - Verify `method == "forward"` and `args.size() == 1`.
   - Verify `arg[0]` type equals `model_sig.input_type`.
   - Return `model_sig.output_type`.

## 4. Verification Step
Compile with `cmake --build build` and test that mismatched layer sizes (e.g., `Dense(4, 8, relu)` followed by `Dense(6, 2, softmax)`) or invalid `@` dimensions produce a clear `TypeError` with exact line and column numbers.
