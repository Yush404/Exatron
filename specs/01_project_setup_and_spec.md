# Feature 01: Project Setup, CMake Configuration, and Exatron Language Specification

## 1. Objective
Set up the project directory hierarchy, CMake build configuration for C++23, and verify the base compiler binary builds cleanly on Arch Linux (`g++ 16+`, `cmake 3.20+`). Establish the complete language specification and EBNF grammar for **Exatron** (`.ex`), a hybrid general-purpose programming and neural network description language that transpiles to standalone C++23 source code.

## 2. Target Directory Structure
Create the following project layout inside the workspace root:

```text
exatron/
├── CMakeLists.txt
├── include/
│   ├── token.hpp
│   ├── lexer.hpp
│   ├── ast.hpp
│   ├── parser.hpp
│   ├── type_checker.hpp
│   ├── runtime_emitter.hpp
│   └── codegen.hpp
├── src/
│   ├── main.cpp
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── type_checker.cpp
│   └── codegen.cpp
└── tests/
    ├── test_core.ex
    ├── test_nn.ex
    └── run_tests.sh
```

## 3. CMake Configuration (`CMakeLists.txt`)
Write `CMakeLists.txt` at the root of the workspace with the following exact configuration:

```cmake
cmake_minimum_required(version 3.20)
project(Exatron VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

file(GLOB_RECURSE EXATRON_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(exatron ${EXATRON_SOURCES})
target_include_directories(exatron PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

## 4. Exatron Language Specification (`.ex`)

### 4.1 Overview
Exatron combines imperative general-purpose constructs (variables, typed arithmetic, conditionals, loops, functions) with first-class neural network primitives (`tensor` types, `@` matrix multiplication, and `model` / `layer` declarations that automatically synthesize forward-pass structs in C++).

### 4.2 Data Types
- `int`: 64-bit signed integer (`int64_t` in C++).
- `float`: 32-bit floating-point scalar (`float` in C++).
- `bool`: Boolean (`bool` in C++: `true`, `false`).
- `string`: UTF-8 string (`std::string` in C++).
- `void`: Return type for functions that do not return a value.
- `tensor<D1, D2, ...>`: Statically shaped 1D or 2D floating-point tensor (e.g., `tensor<4>`, `tensor<2, 3>`).

### 4.3 Statements & Declarations
1. **Variable Declaration**:
   ```text
   let x: int = 42;
   let lr: float = 0.01;
   let weights: tensor<2, 3> = [[0.1, 0.2, 0.3], [0.4, 0.5, 0.6]];
   let zeros_mat: tensor<3, 3> = zeros(3, 3);
   let rand_mat: tensor<2, 4> = rand(2, 4);
   ```
2. **Assignment**:
   ```text
   x = x + 1;
   ```
3. **Control Flow**:
   ```text
   if (x > 10) {
       print(x);
   } else {
       print(0);
   }

   while (x < 100) {
       x = x + 10;
   }

   for (i in 0..10) {
       print(i);
   }
   ```
4. **Functions**:
   ```text
   fn compute_loss(pred: float, target: float) -> float {
       let diff: float = pred - target;
       return diff * diff;
   }
   ```
5. **Neural Network Model Declaration**:
   ```text
   model Classifier {
       input: tensor<1, 4>;
       layer fc1: Dense(4, 8, relu);
       layer fc2: Dense(8, 2, softmax);
   }
   ```
   Instantiating and running a model inside `fn main() -> int`:
   ```text
   let net: Classifier = Classifier();
   let sample: tensor<1, 4> = [[1.0, 0.5, -0.2, 0.8]];
   let out: tensor<1, 2> = net.forward(sample);
   print(out);
   ```

### 4.4 Operators & Precedence (Highest to Lowest)
1. Primary: literals, identifiers, parenthesized `(expr)`, tensor literals `[[...]]`, member call `obj.forward(arg)`, function calls `fn(args)`.
2. Unary: `-`, `!`
3. Multiplicative & Matrix Multiply: `*`, `/`, `%`, `@` (matrix multiplication for 2D tensors)
4. Additive: `+`, `-`
5. Comparison: `<`, `<=`, `>`, `>=`
6. Equality: `==`, `!=`
7. Logical AND: `&&`
8. Logical OR: `||`

### 4.5 Built-in Functions & Activations
- `print(expr)`: Prints any scalar (`int`, `float`, `bool`, `string`) or `tensor` to `stdout` followed by a newline.
- `zeros(rows, cols)`: Returns a 2D `tensor<rows, cols>` filled with `0.0f`.
- `ones(rows, cols)`: Returns a 2D `tensor<rows, cols>` filled with `1.0f`.
- `rand(rows, cols)`: Returns a 2D `tensor<rows, cols>` initialized with deterministic pseudo-random weights in `[-0.5, 0.5]`.
- Activation identifiers inside `Dense(in_features, out_features, activation)`: `relu`, `sigmoid`, `tanh`, `softmax`, `linear`.

### 4.6 Formal EBNF Grammar
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
TensorType     ::= "tensor" "<" INT_LIT ("," INT_LIT)* ">"

Block          ::= "{" Stmt* "}"
Stmt           ::= LetStmt | AssignStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt | PrintStmt | ExprStmt
LetStmt        ::= "let" IDENT ":" Type "=" Expr ";"
AssignStmt     ::= IDENT "=" Expr ";"
IfStmt         ::= "if" "(" Expr ")" Block ("else" Block)?
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
Primary        ::= INT_LIT | FLOAT_LIT | STRING_LIT | "true" | "false" | IDENT | TensorLit | "(" Expr ")"
TensorLit      ::= "[" Expr ("," Expr)* "]"
```

## 5. Initial Bootstrap Verification
1. Create the folders `src`, `include`, and `tests`.
2. Create `CMakeLists.txt` as specified above.
3. Create a minimal `src/main.cpp` that prints `"Exatron compiler v0.1\n"` and returns `0`.
4. Run `cmake -B build && cmake --build build` and run `./build/exatron` to confirm the C++23 toolchain works before proceeding to Feature 02.
