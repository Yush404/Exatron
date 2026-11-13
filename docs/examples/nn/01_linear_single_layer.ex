// 01_linear_single_layer.ex
// A single Dense layer with linear activation: 3 inputs -> 1 output.
// This shape models a simple linear regression: y = x @ W + b.

model LinearRegressor {
    input: tensor<1, 3>;
    layer out: Dense(3, 1, linear);
}

fn main() -> int {
    let net: LinearRegressor = LinearRegressor();
    let x: tensor<1, 3> = [[1.0, 2.0, 3.0]];

    print("Linear Regressor: 3 inputs -> 1 output");
    print("Input:");
    print(x);

    let y: tensor<1, 1> = net.forward(x);
    print("Output (untrained, random weights):");
    print(y);

    return 0;
}

