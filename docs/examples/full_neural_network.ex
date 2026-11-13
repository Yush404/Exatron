model PerceptronClassifier {
    input: tensor<1, 4>;
    layer fc1: Dense(4, 8, relu);
    layer fc2: Dense(8, 4, tanh);
    layer fc3: Dense(4, 3, softmax);
}

fn main() -> int {
    let net: PerceptronClassifier = PerceptronClassifier();
    let features: tensor<1, 4> = [[0.8, -0.5, 1.2, 0.1]];
    let probabilities: tensor<1, 3> = net.forward(features);

    print("Input feature vector:");
    print(features);

    print("Predicted class probabilities:");
    print(probabilities);

    return 0;
}

