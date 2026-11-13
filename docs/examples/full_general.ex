fn gcd(a: int, b: int) -> int {
    while (b != 0) {
        let temp: int = b;
        b = a % b;
        a = temp;
    }
    return a;
}

fn fibonacci(n: int) -> int {
    if (n <= 0) {
        return 0;
    } else if (n == 1) {
        return 1;
    }
    let prev: int = 0;
    let curr: int = 1;
    for (i in 2..n + 1) {
        let nxt: int = prev + curr;
        prev = curr;
        curr = nxt;
    }
    return curr;
}

fn main() -> int {
    print("GCD of 48 and 18:");
    print(gcd(48, 18));

    print("Fibonacci(10):");
    print(fibonacci(10));
    return 0;
}

