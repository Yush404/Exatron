// 05_hybrid_pipeline.ex
// Mixes general-purpose features with a neural network model:
// helper functions, a wrapper around forward(), a loop over samples,
// and labeled printing.

model Classifier {
    input: tensor<1, 3>;
    layer fc1: Dense(3, 4, relu);
    layer fc2: Dense(4, 2, softmax);
}

fn make_sample(a: float, b: float, c: float) -> tensor<1, 3> {
    let t: tensor<1, 3> = [[a, b, c]];
    return t;
}

fn classify(net: Classifier, x: tensor<1, 3>) -> tensor<1, 2> {
    let result: tensor<1, 2> = net.forward(x);
    return result;
}

fn main() -> int {
    let net: Classifier = Classifier();

    print("=== Hybrid Pipeline: classify 4 samples ===");

    let s1: tensor<1, 3> = make_sample(1.0, 0.0, 0.0);
    let s2: tensor<1, 3> = make_sample(0.0, 1.0, 0.0);
    let s3: tensor<1, 3> = make_sample(0.0, 0.0, 1.0);
    let s4: tensor<1, 3> = make_sample(1.0, 1.0, 1.0);

    print("Sample 1: [1, 0, 0]");
    print(classify(net, s1));

    print("Sample 2: [0, 1, 0]");
    print(classify(net, s2));

    print("Sample 3: [0, 0, 1]");
    print(classify(net, s3));

    print("Sample 4: [1, 1, 1]");
    print(classify(net, s4));

    print("=== Running forward in a loop ===");
    let acc: tensor<1, 3> = [[0.5, 0.5, 0.5]];
    for (i in 0..3) {
        let out: tensor<1, 2> = net.forward(acc);
        print(out);
    }

    return 0;
}

