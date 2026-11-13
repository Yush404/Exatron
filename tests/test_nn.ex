// Hybrid Neural Network & Tensor DSL test for Exatron
model IrisNet {
    input: tensor<1, 4>;
    layer hidden1: Dense(4, 8, relu);
    layer hidden2: Dense(8, 4, tanh);
    layer output_layer: Dense(4, 3, softmax);
}

fn main() -> int {
    let a: tensor<2, 3> = [[1.0, 2.0, 3.0], [4.0, 5.0, 6.0]];
    let b: tensor<3, 2> = [[0.5, 1.0], [1.5, 2.0], [2.5, 3.0]];
    let c: tensor<2, 2> = a @ b;

    print("Matrix multiplication (2x3 @ 3x2 -> 2x2):");
    print(c);

    let net: IrisNet = IrisNet();
    let sample: tensor<1, 4> = [[5.1, 3.5, 1.4, 0.2]];
    let probs: tensor<1, 3> = net.forward(sample);

    print("IrisNet Softmax Output Probabilities (1x3):");
    print(probs);

    return 0;
}

