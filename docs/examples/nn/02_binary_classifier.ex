// 02_binary_classifier.ex
// Binary classifier: 2 -> 4 (relu) -> 1 (sigmoid).
// Sigmoid output is in (0, 1), interpretable as a probability.

model BinaryClassifier {
    input: tensor<1, 2>;
    layer hidden: Dense(2, 4, relu);
    layer out: Dense(4, 1, sigmoid);
}

fn main() -> int {
    let net: BinaryClassifier = BinaryClassifier();

    print("Binary Classifier: 2 -> 4 (relu) -> 1 (sigmoid)");

    let x1: tensor<1, 2> = [[0.0, 0.0]];
    let y1: tensor<1, 1> = net.forward(x1);
    print("Input [0, 0] -> output:");
    print(y1);

    let x2: tensor<1, 2> = [[1.0, 0.0]];
    let y2: tensor<1, 1> = net.forward(x2);
    print("Input [1, 0] -> output:");
    print(y2);

    let x3: tensor<1, 2> = [[0.0, 1.0]];
    let y3: tensor<1, 1> = net.forward(x3);
    print("Input [0, 1] -> output:");
    print(y3);

    let x4: tensor<1, 2> = [[1.0, 1.0]];
    let y4: tensor<1, 1> = net.forward(x4);
    print("Input [1, 1] -> output:");
    print(y4);

    return 0;
}

