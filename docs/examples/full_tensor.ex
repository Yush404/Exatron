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

