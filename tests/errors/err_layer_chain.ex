// EXPECT: expects input dimension 6
model BrokenNet {
    input: tensor<1, 4>;
    layer l1: Dense(4, 8, relu);
    layer l2: Dense(6, 2, softmax);
}

fn main() -> int {
    return 0;
}

