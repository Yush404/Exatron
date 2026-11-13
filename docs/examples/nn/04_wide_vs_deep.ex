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

