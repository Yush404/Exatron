// EXPECT: Matrix multiplication dimension mismatch
fn main() -> int {
    let a: tensor<2, 3> = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]];
    let b: tensor<4, 2> = [[1.0, 2.0], [3.0, 4.0], [5.0, 6.0], [7.0, 8.0]];
    let c: tensor<2, 2> = a @ b;
    return 0;
}

