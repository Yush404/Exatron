model TinyMLP {
    input: tensor<1, 3>;
    layer hidden: Dense(3, 4, relu);
    layer output: Dense(4, 2, softmax);
}

fn main() -> int {
    let net: TinyMLP = TinyMLP();
    let x: tensor<1, 3> = [[0.5, -0.2, 1.0]];
    let probs: tensor<1, 2> = net.forward(x);
    print("Model predictions:");
    print(probs);
    return 0;
}

