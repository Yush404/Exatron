// 03_multiclass_iris_shape.ex
// Iris-shaped multi-class classifier: 4 -> 8 (relu) -> 4 (tanh) -> 3 (softmax).
// Softmax output rows sum to 1.0, representing class probabilities.

model IrisClassifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 4, tanh);
    layer fc3: Dense(4, 3, softmax);
}

fn main() -> int {
    let net: IrisClassifier = IrisClassifier();
    let features: tensor<1, 4> = [[5.1, 3.5, 1.4, 0.2]];

    print("Iris Classifier: 4 -> 8 (relu) -> 4 (tanh) -> 3 (softmax)");
    print("Input features:");
    print(features);

    let probs: tensor<1, 3> = net.forward(features);
    print("Output probabilities (softmax, sum to 1.0):");
    print(probs);

    return 0;
}

