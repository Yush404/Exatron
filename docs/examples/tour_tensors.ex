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

